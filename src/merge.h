#ifndef HDRPLUS_MERGE_H_
#define HDRPLUS_MERGE_H_

#include "Halide.h"

/*
 * merge -- fully merges aligned frames in the temporal and spatial
 * dimension to produce one denoised bayer frame.
 */
Halide::Func merge(Halide::Buffer<uint16_t> imgs, Halide::Func alignment);

#endif
