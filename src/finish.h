#ifndef HDRPLUS_FINISH_H_
#define HDRPLUS_FINISH_H_

#include "Halide.h"

struct WhiteBalance {
	float r;
	float g0;
	float g1;
	float b;
};

typedef uint16_t BlackPoint;
typedef uint16_t WhitePoint;

typedef float Compression;
typedef float Gain;

/*
 * finish -- Applies a series of standard local and global image processing
 * operations to an input mosaicked image, producing a pleasant color output.
 * Input pecifies black-level, white-level and white balance. Additionally,
 * tone mapping is applied to the image, as specified by the input compression
 * and gain amounts. This produces natural-looking brightened shadows, without
 * blowing out highlights. The output values are 8-bit.
 */
Halide::Func finish(Halide::Func input, int width, int height, const BlackPoint bp, const WhitePoint wp, const WhiteBalance &wb, const Compression c, const Gain g);

#endif