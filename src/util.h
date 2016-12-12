#ifndef HDRPLUS_UTIL_H_
#define HDRPLUS_UTIL_H_

#include "Halide.h"

Halide::Func box_down2(Halide::Func input);

Halide::Func gauss_down4(Halide::Func input);

#endif