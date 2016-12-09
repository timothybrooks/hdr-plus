#ifndef HDRPLUS_MERGE_H_
#define HDRPLUS_MERGE_H_

#include "Halide.h"

#define T_SIZE 32
#define T_SIZE_2 16

Halide::Image<uint16_t> merge(Halide::Image<uint16_t> imgs, Halide::Func alignment);

#endif