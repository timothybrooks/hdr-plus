#ifndef HDRPLUS_MERGE_H_
#define HDRPLUS_MERGE_H_

#include "Halide.h"

Halide::Image<uint8_t> merge(Halide::Image<uint8_t> imgs, Halide::Func alignment);

#endif