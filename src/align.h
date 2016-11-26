#ifndef HDRPLUS_ALIGN_H_
#define HDRPLUS_ALIGN_H_

#include "Halide.h"

// I wonder if it would be appropriate to make these images 'const' because they should not be modified...
// same idea applies to the merge and finish components

Halide::Func align(Halide::Image imgs);

#endif