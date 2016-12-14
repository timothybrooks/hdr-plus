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
 * finish -- Applies a series of standard local and global operations to a merged image to produce an attractive output with
 * minimal user input. Takes advantage of noise-reduction through frame-merging to make dark details more distinguishable without
 * blowing out highlights or introducing noise.
 */
Halide::Func finish(Halide::Func input, int width, int height, const BlackPoint bp, const WhitePoint wp, const WhiteBalance &wb, const Compression c, const Gain g);

#endif