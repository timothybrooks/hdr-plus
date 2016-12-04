#ifndef HDRPLUS_ALIGN_H_
#define HDRPLUS_ALIGN_H_

#define TILE_SIZE 16 //Must be a multiple of 2
#define SEARCH_RANGE 4
#define DOWNSAMPLE_FACTOR 4

#include "Halide.h"

// I wonder if it would be appropriate to make these images 'const' because they should not be modified...
// same idea applies to the merge and finish components

Halide::Func align(Halide::Image<uint16_t> imgs);

#endif