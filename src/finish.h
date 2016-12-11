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

Halide::Image<uint8_t> finish(Halide::Image<uint16_t> input, const BlackPoint bp, const WhitePoint wp, const WhiteBalance &wb);

#endif