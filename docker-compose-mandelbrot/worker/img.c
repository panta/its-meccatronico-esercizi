#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "img.h"

/* --------------------------------------------------------------------
 *   MACROS AND CONSTANTS
 * -------------------------------------------------------------------- */

#define MAX_BUF_SIZE 64

/* --------------------------------------------------------------------
 *   CODE
 * -------------------------------------------------------------------- */

img_t *image_new(int width, int height)
{
	size_t data_size = (size_t)((size_t)width * (size_t)height * (size_t)3L);
	img_t *img_and_data = (img_t *) malloc(sizeof(img_t) + data_size);
	if (!img_and_data)
		return NULL;
	img_and_data->width = width;
	img_and_data->height = height;
	img_and_data->data = ((uint8_t*)img_and_data) + sizeof(img_t);
	return img_and_data;
}

void image_destroy(img_t *img)
{
	if (!img)
		return;
	memset(img, 0, sizeof(img_t));
	free(img);
}

void image_set_pixel(img_t *img, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	size_t base_offs = (size_t)3L * ((size_t)y * (size_t)img->width + (size_t)x);
	img->data[base_offs + 0] = r;
	img->data[base_offs + 1] = g;
	img->data[base_offs + 2] = b;
}

void image_get_pixel(img_t *img, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b)
{
	size_t base_offs = (size_t)3L * ((size_t)y * (size_t)img->width + (size_t)x);
	if (r)
		*r = img->data[base_offs + 0];
	if (g)
		*g = img->data[base_offs + 1];
	if (b)
		*b = img->data[base_offs + 2];
}

void image_fill(img_t *img, uint8_t r, uint8_t g, uint8_t b)
{
	for (int y = 0; y < img->height; y++) {
		for (int x = 0; x < img->width; x++) {
			size_t base_offs = (size_t)3L * ((size_t)y * (size_t)img->width + (size_t)x);
			img->data[base_offs + 0] = r;
			img->data[base_offs + 1] = g;
			img->data[base_offs + 2] = b;
		}
	}
}

void image_blit(img_t *dst, img_t *src, int dst_x0, int dst_y0, int dst_w, int dst_h, int src_x0, int src_y0)
{
	if (dst_x0 < 0) dst_x0 = 0;
	if (dst_x0 < 0) dst_x0 = 0;
	if (dst_x0 >= (dst->width - 1))
		dst_x0 = dst->width - 1;
	if (dst_y0 >= (dst->height - 1))
		dst_y0 = dst->height - 1;

	int dst_x1 = dst_x0 + dst_w - 1;
	int dst_y1 = dst_y0 + dst_h - 1;
	if (dst_x1 >= (dst->width - 1))
		dst_x1 = dst->width - 1;
	if (dst_y1 >= (dst->height - 1))
		dst_y1 = dst->height - 1;

	if (src_x0 < 0) src_x0 = 0;
	if (src_y0 < 0) src_y0 = 0;
	if (src_x0 >= (src->width - 1))
		src_x0 = src->width - 1;
	if (src_y0 >= (src->height - 1))
		src_y0 = src->height - 1;

	for (int dst_y = dst_y0; dst_y <= dst_y1; dst_y++) {
		int src_y = dst_y + (src_y0 - dst_y0);
		if (src_y < 0) src_y = 0;
		if (src_y >= (src->height - 1))
			src_y = src->height - 1;
		for (int dst_x = dst_x0; dst_x <= dst_x1; dst_x++) {
			int src_x = dst_x + (src_x0 - dst_x0);
			if (src_x < 0) src_x = 0;
			if (src_x >= (src->width - 1))
				src_x = src->width - 1;

			uint8_t r, g, b;
			image_get_pixel(src, src_x, src_y, &r, &g, &b);
			image_set_pixel(dst, dst_x, dst_y, r, g, b);
		}
	}
}


size_t image_stride_size(img_t *img)
{
	return ((size_t)3 * (size_t)img->width);
}

size_t image_data_size(img_t *img)
{
	return ((size_t)3 * (size_t)img->width * (size_t)img->height);
}
