#ifndef IMG_H
#define IMG_H

#include <stdio.h>
#include <stdint.h>

/* --------------------------------------------------------------------
 *   MACROS AND CONSTANTS
 * -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 *   TYPES
 * -------------------------------------------------------------------- */

typedef struct image {
	int width;
	int height;
	uint8_t *data;
} img_t;

/* --------------------------------------------------------------------
 *   PROTOTYPES
 * -------------------------------------------------------------------- */

img_t *image_new(int width, int height);
void image_destroy(img_t *img);
void image_set_pixel(img_t *img, int x, int y, uint8_t r, uint8_t g, uint8_t b);
void image_get_pixel(img_t *img, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b);
void image_fill(img_t *img, uint8_t r, uint8_t g, uint8_t b);
void image_blit(img_t *dst, img_t *src, int dst_x0, int dst_y0, int dst_w, int dst_h, int src_x0, int src_y0);
size_t image_stride_size(img_t *img);
size_t image_data_size(img_t *img);

#endif /* IMG_H */
