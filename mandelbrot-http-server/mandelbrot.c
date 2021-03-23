#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define HTTPSERVER_IMPL
#include "httpserver.h"

/*
 * for an intro to the Mandelbrot set:
 *   - https://simple.wikipedia.org/wiki/Mandelbrot_set
 *
 * for ppm format, see:
 *   - http://netpbm.sourceforge.net/doc/ppm.html
 *
 * install ImageMagick:
 *   debian/ubuntu: apt-get install imagemagick
 *   macos: brew install imagemagick
 *
 * compile:
 *   $ gcc -std=c99 -Wall -O2 -o mandelbrot mandelbrot.c -lm
 *
 * run:
 *   $ ./mandelbrot
 *
 * and visit:
 *
 *    http://127.0.0.1:8080/800/600/-2/-1/1/1
 */

/* --------------------------------------------------------------------
 *   MACROS AND CONSTANTS
 * -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *   TYPES
 * -------------------------------------------------------------------- */

//               width colonne
//  /-----------------------------------\
//  ppppppppppppppppppppppppppppppppppppp \
//  ppppppppppppppppppppppppppppppppppppp |
//  ppppppppppppppppppppppppppppppppppppp |  height righe
//  ...                                   ..
//  ppppppppppppppppppppppppppppppppppppp /
//
//  numero pixels: width * height
//  ogni pixel è una tripla (R, G, B), ognuna un byte -> 3 byte per pixel

typedef struct image {
	int width;
	int height;
	uint8_t *data;			// RGBRGBRGBRGB...   3 byte (R, G, B) * width * height
} img_t;


/* --------------------------------------------------------------------
 *   CODE
 * -------------------------------------------------------------------- */

img_t *image_new(int width, int height)
{
	img_t *img = (img_t *)malloc(sizeof(img_t));
	if (!img)
		return NULL;
	img->width = width;
	img->height = height;

	size_t imageDataSize = (size_t)width * (size_t)height * (size_t)3;
	img->data = (uint8_t *)malloc(imageDataSize);
	if (!img->data) {
		free(img);
		return NULL;
	}

	return img;
}

void image_destroy(img_t *img) {
	if (img) {
		free(img->data);
		free(img);
	}
}

size_t image_stride(img_t *img) {
	return (size_t)3 * (size_t)img->width;
}

void set_pixel(img_t *img, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	size_t pixelOffset = (size_t)3 * (((size_t)y * (size_t)img->width) + (size_t)x);
	img->data[pixelOffset] = r;
	img->data[pixelOffset + 1] = g;
	img->data[pixelOffset + 2] = b;
}

void get_pixel(img_t *img, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b)
{
	size_t pixelOffset = (size_t)3 * (((size_t)y * (size_t)img->width) + (size_t)x);
	if (r)
		*r = img->data[pixelOffset];
	if (g)
		*g = img->data[pixelOffset + 1];
	if (b)
		*b = img->data[pixelOffset + 2];
}

void image_fill(img_t *img, uint8_t r, uint8_t g, uint8_t b)
{
	for (int y = 0; y < img->height; y++) {
		for (int x = 0; x < img->width; x++) {
			set_pixel(img, x, y, r, g, b);
		}
	}
}

int image_save_png(img_t *img, const char *filename) {
	return stbi_write_png(filename, img->width, img->height, 3, img->data, (int)image_stride(img));
}

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
	// (a+bi)(c+di) = ac + adi + bci + bdi^2 = (ac−bd) + (ad+bc)i
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

#define RESPONSE "" \
"<html lang=en>" \
  "<head>" \
    "<meta charset=utf-8>" \
    "<title>Mandelbrot</title>" \
  "</head>" \
  "<body>" \
    "<p>Visita un url del tipo:</p>" \
    "<ul>" \
      "<li><a href=\"/800/600/-2/-1/1/1\">/800/600/-2/-1/1/1</a></li>" \
      "<li><a href=\"/800/800/-1.2/-0.5/-0.6/0\">/800/800/-1.2/-0.5/-0.6/0</a></li>" \
      "<li><a href=\"/3000/2000/-2/-1/1/1\">/3000/2000/-2/-1/1/1</a></li>" \
      "<li><a href=\"/3000/2000/4000/3000/-1.2/-0.5/0.3/0.5\">/4000/3000/-1.2/-0.5/0.3/0.5</a></li>" \
    "</ul>" \
  "</body>" \
"</html>"
#define MAX_URL_LEN 250

void handle_request(struct http_request_s* request) {
	char url_str[MAX_URL_LEN + 1];

	double c_start_re = -2.0, c_start_im = -1.0, c_end_re = 1.0, c_end_im = 1.0;

    http_string_t url = http_request_target(request);
	memcpy(url_str, url.buf, url.len);
	url_str[url.len] = '\0';
	fprintf(stderr, "url: %s\n", url_str);

	int width = 0, height = 0;
	if (sscanf(url_str, "/%d/%d/%lg/%lg/%lg/%lg",
			&width, &height,
			&c_start_re, &c_start_im, &c_end_re, &c_end_im) < 2) {
		struct http_response_s* response = http_response_init();
		http_response_status(response, 200);
		http_response_header(response, "Content-Type", "text/html");
		http_response_body(response, RESPONSE, strlen(RESPONSE));
		http_respond(request, response);
		return;
	}
	fprintf(stderr, "width:%d height:%d\n", width, height);
	fprintf(stderr, "region (%lg,%lg)-(%lg,%lg)\n", c_start_re, c_start_im, c_end_re, c_end_im);

	img_t *img = image_new(width, height);
	if (!img) {
		fprintf(stderr, "ERROR: can't allocate image\n");
		exit(EXIT_FAILURE);
	}

	image_fill(img, 255, 255, 255);

	double t_start = time_ms();
	double c_re, c_im;
	for (int y = 0; y < img->height; y++) {
		c_im = c_start_im + ((double)y / (double)img->height) * (c_end_im - c_start_im);

		for (int x = 0; x < img->width; x++) {
			c_re = c_start_re + ((double)x / (double)img->width) * (c_end_re - c_start_re);

			int m = mandelbrot(c_re, c_im);
			int color = 255 - (int)((double)m * 255.0 / (double)MAX_ITER);
			set_pixel(img, x, y, (uint8_t)color, (uint8_t)color, (uint8_t)color);
		}
	}
	double t_end = time_ms();
	fprintf(stderr, "calc time: %lg ms\n", (t_end - t_start));

	t_start = time_ms();
	image_save_png(img, "mandelbrot.png");
	t_end = time_ms();
	fprintf(stderr, "write time: %lg ms\n", (t_end - t_start));

	FILE *fp = fopen("mandelbrot.png", "rb");
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *pngData = malloc(fileSize);
	if (!pngData) {
		fprintf(stderr, "ERROR: can't allocate PNG data memory\n");
		exit(EXIT_FAILURE);
	}
	if (fread(pngData, fileSize, 1, fp) != 1) {
		fprintf(stderr, "ERROR: can't read PNG data into memory\n");
		exit(EXIT_FAILURE);
	}
	fclose(fp);
	fprintf(stderr, "file size:%ld\n", fileSize);

	struct http_response_s* response = http_response_init();
	http_response_status(response, 200);
	http_response_header(response, "Content-Type", "image/png");
	http_response_body(response, pngData, fileSize);
	http_respond(request, response);
	free(pngData);
}

int main(void)
{
	struct http_server_s* server = http_server_init(8080, handle_request);
	http_server_listen(server);
}
