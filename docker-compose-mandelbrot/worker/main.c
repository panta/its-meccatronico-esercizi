#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>

#define HTTPSERVER_IMPL
#include "httpserver.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "img.h"

#define DEFAULT_PORT 8000

typedef struct mandelbrot_region {
	double c_start_re;
	double c_start_im;
	double c_end_re;
	double c_end_im;
	int width;
	int height;
} mandelbrot_region_t;


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

#define MAX_ITER 100

double complex_sq_abs(double re, double im)
{
	return (re * re) + (im * im);
}

// time_ms() returns the number of ms since epoch (1 jan 1970)
double time_ms(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return ((t.tv_sec * (double)1000.0) + (t.tv_usec / (double)1000.0));
}

int mandelbrot(double c_re, double c_im)
{
	// z_0 = 0
	// z_{n+1} = (z_n)^2 + c
	// it's in the mandelbrot set if |z_n| < 2 after MAX_ITER

	// |z| = |x+yi| = sqrt(x*x + y*y)
	// (a	+bi)(c+di) = ac + adi + bci + bdi^2 = (ac−bd) + (ad+bc)i
	// z^2 = (x+yi)^2 = (x^2-y^2) + (xy+yx)i = (x^2-y^2) + 2xyi

	double z_re = 0.0, z_im = 0.0;
	double z_new_re = 0.0, z_new_im = 0.0;
	int n = 0;
	while (n < MAX_ITER) {
		if (complex_sq_abs(z_re, z_im) > 4.0)
			break;
		// z_{n+1} = (z_n)^2 + c
		z_new_re = ((z_re * z_re) - (z_im * z_im)) + c_re;
		z_new_im = 2 * z_re * z_im + c_im;

		z_re = z_new_re;
		z_im = z_new_im;
		n++;
	}
	return n;
}

int request_target_is(struct http_request_s* request, char const * target) {
    http_string_t url = http_request_target(request);
    int len = strlen(target);
    return len == url.len && memcmp(url.buf, target, url.len) == 0;
}

#define OOM_RESPONSE "out of memory"
#define NOT_FOUND "not found"
#define INTERNAL_ERROR_RESPONSE "internal error"
#define PATHNAME_LEN 240
#define MAX_URL_SIZE 240

void handle_request(struct http_request_s* request) {
    http_string_t url = http_request_target(request);

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
    fprintf(stderr, "got request for: %d x %d (%lg,%lg)-(%lg,%lg)\n", region.width, region.height, region.c_start_re, region.c_start_im, region.c_end_re, region.c_end_im);
    fflush(stderr);

	img_t *img = image_new(region.width, region.height);
	if (!img) {
        http_response_status(response, 500);
        http_response_header(response, "Content-Type", "text/plain");
        http_response_body(response, OOM_RESPONSE, sizeof(OOM_RESPONSE) - 1);
        http_respond(request, response);
        return;
	}

	image_fill(img, 255, 255, 255);

	double c_re, c_im;
	for (int y = 0; y < img->height; y++) {
		c_im = region.c_start_im + ((double)y / (double)img->height) * (region.c_end_im - region.c_start_im);

		for (int x = 0; x < img->width; x++) {
			c_re = region.c_start_re + ((double)x / (double)img->width) * (region.c_end_re - region.c_start_re);

			int m = mandelbrot(c_re, c_im);
			int color = 255 - (int)((double)m * 255.0 / (double)MAX_ITER);
			image_set_pixel(img, x, y, (uint8_t)color, (uint8_t)color, (uint8_t)color);
		}
	}

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
    http_respond(request, response);
    return;

not_found:
    http_response_status(response, 404);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, NOT_FOUND, sizeof(NOT_FOUND) - 1);
    http_respond(request, response);
    fprintf(stderr, "returned NOT FOUND response for url '%s'\n", url_str);
    fflush(stderr);
    return;

internal_error:
    http_response_status(response, 500);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, INTERNAL_ERROR_RESPONSE, sizeof(INTERNAL_ERROR_RESPONSE) - 1);
    http_respond(request, response);
    fprintf(stderr, "returned FAIL response\n");
    fflush(stderr);
    return;
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
