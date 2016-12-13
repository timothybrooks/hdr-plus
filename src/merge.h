#ifndef HDRPLUS_MERGE_H_
#define HDRPLUS_MERGE_H_

#include "Halide.h"

/*
 *
 */
Halide::Func merge(Halide::Image<uint16_t> imgs, Halide::Func alignment);

#endif