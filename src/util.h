#ifndef HDRPLUS_UTIL_H_
#define HDRPLUS_UTIL_H_

#include "Halide.h"

Halide::Func box_down2(Halide::Func input);

Halide::Func gauss_down4(Halide::Func input);

//Takes in a 2D image
Halide::Func gauss_7x7(Halide::Func input, bool isFloat);

#endif