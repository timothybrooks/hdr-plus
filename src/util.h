#ifndef HDRPLUS_UTIL_H_
#define HDRPLUS_UTIL_H_

#include "Halide.h"

/*
 * box_down2 -- Averages and downsamples input by 2
 */
Halide::Func box_down2(Halide::Func input, std::string name);

/*
 * gauss_down4 -- Blurs and downsamples input by 4
 */
Halide::Func gauss_down4(Halide::Func input, std::string name);

/*
 * gauss_7x7 -- Blurs its input with a 7x7 gaussian kernel. Requires input
 * to handle boundaries. Std dev = 4/3
 */
Halide::Func gauss_7x7(Halide::Func input, std::string name);

/*
 * gauss_15x15 -- Blurs its input with a 15x15 gaussian kernel. Requires input
 * to handle boundaries. Std dev = 8/3
 */
Halide::Func gauss_15x15(Halide::Func input, std::string name);

/*
 * diff -- Computes difference between two integer functions
 */
Halide::Func diff(Halide::Func im1, Halide::Func im2, std::string);

/*
 * gamma_correct -- Takes a single or multi-channel linear image and applies gamma correction
 * as described here: http://www.color.org/sRGB.xalter. See formulas 1.2a and 1.2b
 */
Halide::Func gamma_correct(Halide::Func input);

/*
 * gamma_inverse -- Takes a single or multi-channel image and undoes gamma correction to 
 * return in to linear RGB space.
 */
Halide::Func gamma_inverse(Halide::Func input);

/*
 * rgb_to_yuv -- converts a u16 linear rgb image to an f32 linear yuv image.
 */
 Halide::Func rgb_to_yuv(Halide::Func input);

/*
 * yuv_to_rgb -- Converts f32 YUV image to u16 RGB linear image
 */
 Halide::Func yuv_to_rgb(Halide::Func input);

#endif