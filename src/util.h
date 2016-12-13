#ifndef HDRPLUS_UTIL_H_
#define HDRPLUS_UTIL_H_

#include "Halide.h"

/*
 * Averages and downsamples input by 2
 */
Halide::Func box_down2(Halide::Func input, std::string name);

/*
 * Blurs and downsamples input by 4
 */
Halide::Func gauss_down4(Halide::Func input, std::string name);

/*
 * Blurs its input with a 5x5 gaussian kernel. Requires input
 * to handle edge cases.
 */
Halide::Func gauss_5x5(Halide::Func input, std::string name);

/*
 * Blurs its input with a 7x7 gaussian kernel. Requires input
 * to handle edge cases.
 */
Halide::Func gauss_7x7(Halide::Func input, std::string name);

/*
 * Utility to take the difference between two functions
 */
Halide::Func diff(Halide::Func im1, Halide::Func im2, std::string);

Halide::Func gamma_correct(Halide::Func input);

Halide::Func gamma_inverse(Halide::Func input);

/*
 * Converts u16 RGB linear image to f32 YUV image
 */
 Halide::Func rgb_to_yuv(Halide::Func input);

/*
 * Converts f32 YUV image to u16 RGB linear image
 */
 Halide::Func yuv_to_rgb(Halide::Func input);

#endif