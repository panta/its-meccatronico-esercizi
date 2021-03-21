#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>

#define HTTP_IMPLEMENTATION
#include "http.h"

#define HTTP_REQUEST_TIMEOUT 240

#define HTTPSERVER_IMPL
#include "httpserver.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "img.h"

#define DEFAULT_PORT 9000

#define MAX_WORKERS 100
#define DEFAULT_WORKER_BASE_NAME "worker"
#ifdef LOCAL_USE
    #define DEFAULT_NWORKER_H 1
    #define DEFAULT_NWORKER_V 1
#else
    #define DEFAULT_NWORKER_H 3
    #define DEFAULT_NWORKER_V 3
#endif

#ifndef TRUE
#define FALSE ((int)0)
#define TRUE ((int)1)
#endif /* TRUE */

typedef struct mandelbrot_region {
	double c_start_re;
	double c_start_im;
	double c_end_re;
	double c_end_im;
	int width;
	int height;
} mandelbrot_region_t;

typedef struct worker {
    int worker_no;
    int row;
    int column;
    mandelbrot_region_t region;
    http_t *request;
    http_status_t status;
    int received;
} worker_t;


/*
 * for an intro to the Mandelbrot set:
 *   - https://simple.wikipedia.org/wiki/Mandelbrot_set
 *
 * compile:
 *   $ gcc -std=c99 -Wall -o worker main.c img.c -lm
 *
 * run:
 *   $ ./worker
 *
 * and visit: http://127.0.0.1:8000/600/400/-2/-1/1/1
 */

// time_ms() returns the number of ms since epoch (1 jan 1970)
double time_ms(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return ((t.tv_sec * (double)1000.0) + (t.tv_usec / (double)1000.0));
}

#define OOM_RESPONSE "out of memory"
#define NOT_FOUND "not found"
#define INTERNAL_ERROR_RESPONSE "internal error"
#define PATHNAME_LEN 240
#define MAX_URL_SIZE 240

int merge_worker_image(img_t *dst, mandelbrot_region_t *dst_region, worker_t *worker);

void handle_request(struct http_request_s* srv_request) {
    http_string_t url = http_request_target(srv_request);

    struct http_response_s* response = http_response_init();

    char url_str[MAX_URL_SIZE + 1];
    memcpy(url_str, url.buf, url.len);
    url_str[url.len] = '\0';
    fprintf(stderr, "url '%s'\n", url_str);

    mandelbrot_region_t region;
    memset(&region, 0, sizeof(region));
    if (sscanf(url.buf, "/%d/%d/%lg/%lg/%lg/%lg",
            &region.width, &region.height,
            &region.c_start_re, &region.c_start_im,
            &region.c_end_re, &region.c_end_im) != 6) {
        goto not_found;
    }
    fprintf(stderr, "%d x %d (%lg,%lg)-(%lg,%lg)\n", region.width, region.height, region.c_start_re, region.c_start_im, region.c_end_re, region.c_end_im);

	img_t *img = image_new(region.width, region.height);
	if (!img) {
        http_response_status(response, 500);
        http_response_header(response, "Content-Type", "text/plain");
        http_response_body(response, OOM_RESPONSE, sizeof(OOM_RESPONSE) - 1);
        http_respond(srv_request, response);
        return;
	}

	image_fill(img, 255, 255, 255);

    int nworkers_h = DEFAULT_NWORKER_H;
    int nworkers_v = DEFAULT_NWORKER_V;
    int nworkers = nworkers_v * nworkers_h;

    worker_t worker[MAX_WORKERS];

    for (int i = 0; i < nworkers; i++) {
        memset(&worker[i], 0, sizeof(worker_t));
        worker[i].request = NULL;
        worker[i].status = HTTP_STATUS_PENDING;
        worker[i].received = -1;
    }

    double region_c_w = (region.c_end_re - region.c_start_re) / (double)nworkers_h;
    double region_c_h = (region.c_end_im - region.c_start_im) / (double)nworkers_v;
    int n_running = 0;
    for (int i = 0; i < nworkers_v; i++) {
        for (int j = 0; j < nworkers_h; j++) {
            int worker_no = (i * nworkers_h) + j;
            worker_t *worker_ptr = &worker[worker_no];

            worker_ptr->worker_no = worker_no;
            worker_ptr->row = i;
            worker_ptr->column = j;
            mandelbrot_region_t *w_region_ptr = &(worker_ptr->region);
            w_region_ptr->width = region.width / nworkers_h;
            w_region_ptr->height = region.height / nworkers_v;
            w_region_ptr->c_start_re = region.c_start_re + (region_c_w * (double)j);
            w_region_ptr->c_end_re   = w_region_ptr->c_start_re + region_c_w;
            w_region_ptr->c_start_im = region.c_start_im + (region_c_h * (double)i);
            w_region_ptr->c_end_im   = w_region_ptr->c_start_im + region_c_h;

            char url[MAX_URL_SIZE + 1];
#ifdef LOCAL_USE
            sprintf(url, "http://127.0.0.1:8000/%d/%d/%lf/%lf/%lf/%lf",
                w_region_ptr->width, w_region_ptr->height,
                w_region_ptr->c_start_re, w_region_ptr->c_start_im,
                w_region_ptr->c_end_re, w_region_ptr->c_end_im);
#else
            sprintf(url, "http://%s%d:8000/%d/%d/%lf/%lf/%lf/%lf", DEFAULT_WORKER_BASE_NAME, worker_no,
                w_region_ptr->width, w_region_ptr->height,
                w_region_ptr->c_start_re, w_region_ptr->c_start_im,
                w_region_ptr->c_end_re, w_region_ptr->c_end_im);
#endif
            fprintf(stderr, "making request to url %s\n", url);
            http_t *request = http_get(url, NULL);
            worker_ptr->request = request;
            if (!request) {
                fprintf(stderr, "Invalid request for worker %d (%dx%d)\n", worker_no, j, i);
                // goto bad_request;
            }
            n_running++;
        }
    }

    while (1) {
        int n_completed = 0;
        int n_failed = 0;
        int n_pending = 0;
        int n_transitions = 0;
        for (int i = 0; i < nworkers; i++) {
            worker_t *worker_ptr = &worker[i];
            http_t *request = worker_ptr->request;
            if (!request)
                continue;
            http_status_t status = http_process(request);
            if (status == HTTP_STATUS_PENDING) {
                n_pending++;
            } else if (status == HTTP_STATUS_FAILED) {
                n_failed++;
            } else if (status == HTTP_STATUS_COMPLETED) {
                n_completed++;
            }
            if (status != worker_ptr->status) {
                worker_ptr->status = status;
                n_transitions++;
                if (status == HTTP_STATUS_FAILED) {
                    fprintf(stderr, "worker[%d] status:FAILED  [%d] %s\n", i, (int)request->status_code, request->reason_phrase);
                } else if (status == HTTP_STATUS_COMPLETED) {
                    fprintf(stderr, "worker[%d] status:COMPLETED  received:%d\n", i, (int)request->response_size);
                    if (!merge_worker_image(img, &region, worker_ptr)) {
                        fprintf(stderr, "worker[%d] merge FAILED\n", i);
                    }
                }
            }
        }
        if (n_transitions > 0) {
            fprintf(stderr, "pending/failed/completed: %d/%d/%d\n", n_pending, n_failed, n_completed);
        }
        if (n_pending == 0) {
            break;
        }
    }

    fprintf(stderr, "finishing handler work...\n");

    for (int i = 0; i < nworkers; i++) {
        worker_t *worker_ptr = &worker[i];
        http_t *request = worker_ptr->request;
        if (!request)
            continue;
        http_release(request);
    }

    // http_response_status(response, 200);
    // http_response_header(response, "Content-Type", "text/plain");
    // http_response_body(response, "OK", 2);
    // http_respond(srv_request, response);

    char filename[PATHNAME_LEN + 1];
    strcpy(filename, "/tmp/mandelXXXXXX.png");
    int fd = mkstemp(filename);
    if (fd >= 0)
        close(fd);
    if (strcmp(filename, "") == 0) {
        strcpy(filename, "/tmp/mandeltmp.png");
    }
    if (!stbi_write_png(filename, img->width, img->height, 3, img->data, image_stride_size(img))) {
        goto internal_error;
    }
 
    size_t png_size = 0;
    struct stat statbuf;
    if (stat(filename, &statbuf) < 0) {
        perror("stat");
        goto internal_error;
    }
    png_size = statbuf.st_size;

    char *buffer = malloc(png_size);
    if (!buffer) {
        goto internal_error;
    }
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        goto internal_error;
    }
    if (fread(buffer, png_size, 1, fp) != 1) {
        perror("fread");
        goto internal_error;
    }
    fclose(fp);
    fp = NULL;

    http_response_status(response, 200);
    http_response_header(response, "Content-Type", "image/png");
    http_response_body(response, buffer, png_size);
    http_respond(srv_request, response);
    return;

not_found:
    http_response_status(response, 404);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, NOT_FOUND, sizeof(NOT_FOUND) - 1);
    http_respond(srv_request, response);
    fprintf(stderr, "returned NOT FOUND response for url '%s'\n", url_str);
    fflush(stderr);
    return;

internal_error:
    http_response_status(response, 500);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, INTERNAL_ERROR_RESPONSE, sizeof(INTERNAL_ERROR_RESPONSE) - 1);
    http_respond(srv_request, response);
    return;
}

int merge_worker_image(img_t *dst, mandelbrot_region_t *dst_region, worker_t *worker)
{
    http_t *worker_request = worker->request;
    mandelbrot_region_t *worker_region = &(worker->region);
    int width = 0, height = 0, channels = 0;

    const unsigned char *data = stbi_load_from_memory((const unsigned char *) worker_request->response_data, (int) worker_request->response_size,
            &width, &height, &channels, 3);
    if (!data) {
        fprintf(stderr, "can't load image from worker %d\n", worker->worker_no);
        return FALSE;
    }
    if (channels != 3) {
        fprintf(stderr, "got different number of channels (%d)\n", channels);
        return FALSE;
    }

    img_t *src = image_new(worker_region->width, worker_region->height);
    if (! src) {
        fprintf(stderr, "can't create image for worker %d results\n", worker->worker_no);
        return FALSE;
    }

    memcpy(src->data, data, image_data_size(src));

    stbi_image_free((void *)data);

    int dst_x0 = worker->column * worker_region->width;
    int dst_y0 = worker->row * worker_region->height;
    image_blit(dst, src, dst_x0, dst_y0, worker_region->width, worker_region->height, 0, 0);
    return TRUE;
}

void sig_handler(int signum) {
    fprintf(stderr, "exiting...\n");
    exit(EXIT_FAILURE);
}

int main(void) {
    int port = DEFAULT_PORT;

    const char *port_str = getenv("PORT");
	if (port_str && (strcmp(port_str, "") != 0)) {
		port = atoi(port_str);
	}

    signal(SIGINT, sig_handler);

    fprintf(stderr, "listening on port %d...\n", port);
    struct http_server_s* server = http_server_init(port, handle_request);
    http_server_listen(server);
}
