#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <pthread.h>

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
 *   $ gcc -std=c99 -Wall -Wextra -pedantic -Wstrict-prototypes -Wconversion -Werror -o mandelbrot mandelbrot.c
 *
 * run:
 *   $ ./mandelbrot >mandelbrot.ppm && convert mandelbrot.ppm mandelbrot.jpg
 */

typedef void *(*thread_func_t) (void *);

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, thread_func_t func, void *arg);

/* --------------------------------------------------------------------
 *   MACROS AND CONSTANTS
 * -------------------------------------------------------------------- */

#define MAX_THREADS 128

#define WIDTH	4000
#define HEIGHT	3000

/* --------------------------------------------------------------------
 *   TYPES
 * -------------------------------------------------------------------- */

typedef enum {
	RED = 0,
	GREEN = 1,
	BLUE = 2,
} ch_t;

typedef uint8_t img_channel_t[HEIGHT][WIDTH];
typedef img_channel_t img_t[3];

img_t img;

typedef struct work {
	int x0, y0;					// angolo in alto a sx
	int x1, y1;					// angolo in basso a dx
} work_t;

/* --------------------------------------------------------------------
 *   CODE
 * -------------------------------------------------------------------- */

void set_pixel(img_t img, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	img[RED][y][x]   = r;
	img[GREEN][y][x] = g;
	img[BLUE][y][x]  = b;
}

#define MAX_ITER 100

double complex_sq_abs(double re, double im)
{
	return (re * re) + (im * im);
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

// time_ms() returns the number of ms since epoch (1 jan 1970)
double time_ms(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return ((t.tv_sec * (double)1000.0) + (t.tv_usec / (double)1000.0));
}

void *thread_mandelbrot(void *data)
{
	double c_start_re = -2.0, c_start_im = -1.0, c_end_re = 1.0, c_end_im = 1.0;
	// work_t *work = (work_t *)data;

	int x0 = ((work_t *)data)->x0;
	int y0 = ((work_t *)data)->y0;
	int x1 = ((work_t *)data)->x1;
	int y1 = ((work_t *)data)->y1;

	double c_re, c_im;
	for (int y = y0; y <= y1; y++) {
		c_im = c_start_im + ((double)y / (double)HEIGHT) * (c_end_im - c_start_im);

		for (int x = x0; x <= x1; x++) {
			c_re = c_start_re + ((double)x / (double)WIDTH) * (c_end_re - c_start_re);

			int m = mandelbrot(c_re, c_im);
			int color = 255 - (int)((double)m * 255.0 / (double)MAX_ITER);
			set_pixel(img, x, y, (uint8_t)color, (uint8_t)color, (uint8_t)color);
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	work_t work[MAX_THREADS];
	pthread_t thread[MAX_THREADS];
	int h_threads = 1;
	int v_threads = 1;

	if (argc > 1) {
		h_threads = atoi(argv[1]);
	}
	if (argc > 2) {
		v_threads = atoi(argv[2]);
	}

	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			set_pixel(img, x, y, 255, 255, 255);
		}
	}

	// lanciamo i thread
	double t_start = time_ms();
	int w_slice = WIDTH / h_threads;
	int h_slice = HEIGHT / v_threads;
	for (int i = 0; i < v_threads; i++) {
		for (int j = 0; j < h_threads; j++) {
			int x0 = w_slice * j;
			int y0 = h_slice * i;
			int x1 = x0 + w_slice - 1;		// w_slice * (i+1) - 1
			int y1 = y0 + h_slice - 1;

			int thread_index = (i * h_threads) + j;
			work[thread_index].x0 = x0;
			work[thread_index].y0 = y0;
			work[thread_index].x1 = x1;
			work[thread_index].y1 = y1;

			pthread_create(&thread[thread_index], NULL, &thread_mandelbrot, &work[thread_index]);
		}
	}

	// aspettiamo che finiscano tutti i thread
	for (int i = 0; i < v_threads; i++) {
		for (int j = 0; j < h_threads; j++) {
			int thread_index = (i * h_threads) + j;
			pthread_join(thread[thread_index], NULL);
		}
	}
	double t_end = time_ms();
	fprintf(stderr, "calc time: %lg ms\n", (t_end - t_start));

	// scriviamo l'immagine sull'output

	t_start = time_ms();
	printf("P6\n");
	// printf("# mandelbrot.ppm\n");
	printf("%d %d\n", WIDTH, HEIGHT);
	printf("%d\n", 255);
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			uint8_t pixel[3];
			pixel[0] = img[RED][y][x];
			pixel[1] = img[GREEN][y][x];
			pixel[2] = img[BLUE][y][x];
			fwrite(pixel, sizeof(pixel), 1, stdout);
			// printf(" %d %d %d  ", img[RED][y][x], img[GREEN][y][x], img[BLUE][y][x]);
		}
		// printf("\n");
	}
	t_end = time_ms();
	fprintf(stderr, "write time: %lg ms\n", (t_end - t_start));
}
