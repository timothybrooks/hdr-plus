#include "finish.h"

#include "Halide.h"
#include "halide_image_io.h"
#include "util.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

/*
 * black_white_point -- renormalizes an image based on a precomputed black and white
 * point to take advantage of the full 16-bit integer depth.
 */
Func black_white_point(Func input, const BlackPoint bp, const BlackPoint wp) {

    Func output("black_white_point_output");

    Var x, y;

    float white_factor = 65535.f / (wp - bp);

    output(x, y) = u16_sat((i32(input(x, y)) - bp) * white_factor);

    return output;
}

/*
 * white-balance -- white-balances the color channels of an image based on precomputed 
 * data stored with the RAW photo. Note that the two green channels in the bayer pattern
 * are white-balanced separately. 
 */
Func white_balance(Func input, int width, int height, const WhiteBalance &wb) {

    Func output("white_balance_output");

    Var x, y;
    RDom r(0, width / 2, 0, height / 2);            // reduction over half of image

    output(x, y) = u16(0);

    output(r.x * 2    , r.y * 2    ) = u16_sat(wb.r  * f32(input(r.x * 2    , r.y * 2    )));   // red
    output(r.x * 2 + 1, r.y * 2    ) = u16_sat(wb.g0 * f32(input(r.x * 2 + 1, r.y * 2    )));   // green 0
    output(r.x * 2    , r.y * 2 + 1) = u16_sat(wb.g1 * f32(input(r.x * 2    , r.y * 2 + 1)));   // green 1
    output(r.x * 2 + 1, r.y * 2 + 1) = u16_sat(wb.b  * f32(input(r.x * 2 + 1, r.y * 2 + 1)));   // blue

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    output.update(0).parallel(r.y);
    output.update(1).parallel(r.y);
    output.update(2).parallel(r.y);
    output.update(3).parallel(r.y);

    return output;
}

/*
 * demosaic -- interpolates missing color channels in the bayer mosaic based on the work of Malvar et. al described here:
 * https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/Demosaicing_ICASSP04.pdf . Assumes that data is laid
 * out in an RGGB pattern.
 */
Func demosaic(Func input, int width, int height) {

    // assumes RG/GB Bayer pattern

    Func f0("demosaic_f0");             // G at R locations; G at B locations
    Func f1("demosaic_f1");             // R at green in R row, B column; B at green in B row, R column
    Func f2("demosaic_f2");             // R at green in B row, R column; B at green in R row, B column
    Func f3("demosaic_f3");             // R at blue in B row, B column; B at red in R row, R column

    Func d0("demosaic_0");
    Func d1("demosaic_1");
    Func d2("demosaic_2");
    Func d3("demosaic_3");

    Func output("demosaic_output");

    Var x, y, c;
    RDom r0(-2, 5, -2, 5);                          // reduction over weights in filter
    RDom r1(0, width / 2, 0, height / 2);           // reduction over half of image

    // mirror input image with overlapping edges to keep mosaic pattern consistency

    Func input_mirror = BoundaryConditions::mirror_interior(input, 0, width, 0, height);

    // demosaic filters

    f0(x,y) = 0;
    f1(x,y) = 0;
    f2(x,y) = 0;
    f3(x,y) = 0;

    int f0_sum = 8;
    int f1_sum = 16;
    int f2_sum = 16;
    int f3_sum = 16;

                                    f0(0, -2) = -1;
                                    f0(0, -1) =  2;
    f0(-2, 0) = -1; f0(-1, 0) = 2;  f0(0,  0) =  4; f0(1, 0) = 2;   f0(2, 0) = -1;
                                    f0(0,  1) =  2;
                                    f0(0,  2) = -1;

                                    f1(0, -2) =  1;
                    f1(-1,-1) = -2;                 f1(1, -1) = -2;
    f1(-2, 0) = -2; f1(-1, 0) =  8; f1(0,  0) = 10; f1(1,  0) =  8; f1(2, 0) = -2;
                    f1(-1, 1) = -2;                 f1(1,  1) = -2;
                                    f1(0,  2) =  1;

                                    f2(0, -2) = -2;
                    f2(-1,-1) = -2; f2(0, -1) =  8; f2(1, -1) = -2;
    f2(-2, 0) = 1;                  f2(0,  0) = 10;                 f2(2, 0) = 1;
                    f2(-1, 1) = -2; f2(0,  1) =  8; f2(1,  1) = -2;
                                    f2(0,  2) = -2;

                                    f3(0, -2) = -3;
                    f3(-1,-1) = 4;                  f3(1, -1) = 4;
    f3(-2, 0) = -3;                 f3(0,  0) = 12;                 f3(2, 0) = -2;
                    f3(-1, 1) = 4;                  f3(1,  1) = 4;
                                    f3(0,  2) = -3;

    // intermediate demosaic functions

    d0(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f0(r0.x, r0.y)) / f0_sum);
    d1(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f1(r0.x, r0.y)) / f1_sum);
    d2(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f2(r0.x, r0.y)) / f2_sum);
    d3(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f3(r0.x, r0.y)) / f3_sum);

    // resulting demosaicked function    

    output(x, y, c) = input(x, y);                                              // initialize each channel to input mosaicked image

    // red
    output(r1.x * 2 + 1, r1.y * 2,     0) = d1(r1.x * 2 + 1, r1.y * 2);         // R at green in R row, B column
    output(r1.x * 2,     r1.y * 2 + 1, 0) = d2(r1.x * 2,     r1.y * 2 + 1);     // R at green in B row, R column
    output(r1.x * 2 + 1, r1.y * 2 + 1, 0) = d3(r1.x * 2 + 1, r1.y * 2 + 1);     // R at blue in B row, B column

    // green
    output(r1.x * 2,     r1.y * 2,     1) = d0(r1.x * 2,     r1.y * 2);         // G at R locations
    output(r1.x * 2 + 1, r1.y * 2 + 1, 1) = d0(r1.x * 2 + 1, r1.y * 2 + 1);     // G at B locations

    // blue
    output(r1.x * 2,     r1.y * 2 + 1, 2) = d1(r1.x * 2,     r1.y * 2 + 1);     // B at green in B row, R column
    output(r1.x * 2 + 1, r1.y * 2,     2) = d2(r1.x * 2 + 1, r1.y * 2);         // B at green in R row, B column
    output(r1.x * 2,     r1.y * 2,     2) = d3(r1.x * 2,     r1.y * 2);         // B at red in R row, R column

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    f0.compute_root().parallel(y).parallel(x);
    f1.compute_root().parallel(y).parallel(x);
    f2.compute_root().parallel(y).parallel(x);
    f3.compute_root().parallel(y).parallel(x);

    d0.compute_root().parallel(y).vectorize(x, 16);
    d1.compute_root().parallel(y).vectorize(x, 16);
    d2.compute_root().parallel(y).vectorize(x, 16);
    d3.compute_root().parallel(y).vectorize(x, 16);

    output.compute_root().parallel(y).vectorize(x, 16);

    output.update(0).parallel(r1.y);
    output.update(1).parallel(r1.y);
    output.update(2).parallel(r1.y);
    output.update(3).parallel(r1.y);
    output.update(4).parallel(r1.y);
    output.update(5).parallel(r1.y);
    output.update(6).parallel(r1.y);
    output.update(7).parallel(r1.y);

    return output;
}

/*
 * combine -- combines two greyscale images with a laplacian pyramid by using the given 
 * distribution function to weight the images relative to each other.
 */
Func combine(Func im1, Func im2, int width, int height, Func dist) {

    // exposure fusion as described by Mertens et al. modified to only use intensity metric
    // http://ntp-0.cs.ucl.ac.uk/staff/j.kautz/publications/exposure_fusion.pdf

    Func init_mask1("mask1_layer_0");
    Func init_mask2("mask2_layer_0");
    Func accumulator("combine_accumulator");
    Func output("combine_output");

    Var x, y;

    // mirror input images

    Func im1_mirror = BoundaryConditions::repeat_edge(im1, 0 , width, 0, height);
    Func im2_mirror = BoundaryConditions::repeat_edge(im2, 0 , width, 0, height);

    // initial blurred layers to compute laplacian pyramid

    Func unblurred1 = im1_mirror;
    Func unblurred2 = im2_mirror;

    Func blurred1 = gauss_7x7(im1_mirror, "img1_layer_0");
    Func blurred2 = gauss_7x7(im2_mirror, "img2_layer_0");

    Func laplace1, laplace2, mask1, mask2;

    // initial masks computed from input distribution function

    Expr weight1 = f32(dist(im1_mirror(x,y)));
    Expr weight2 = f32(dist(im2_mirror(x,y)));

    init_mask1(x, y) = weight1 / (weight1 + weight2);
    init_mask2(x, y) = 1.f - init_mask1(x, y);

    mask1 = init_mask1;
    mask2 = init_mask2;

    // blend frequency band of images with corresponding frequency band of weights; accumulate over frequency bands

    int num_layers = 3;

    accumulator(x, y) = i32(0);

    for (int layer = 1; layer < num_layers; layer++) {

        std::string prev_layer_str = std::to_string(layer - 1);
        std::string layer_str = std::to_string(layer);

        // previous laplace layer

        laplace1 = diff(unblurred1, blurred1, "laplace1_layer_" + prev_layer_str);
        laplace2 = diff(unblurred2, blurred2, "laplace2_layer_" + layer_str);

        // add previous frequency band

        accumulator(x, y) += i32(laplace1(x,y) * mask1(x,y)) + i32(laplace2(x,y) * mask2(x,y));

        // save previous gauss layer to produce current laplace layer

        unblurred1 = blurred1;
        unblurred2 = blurred2;

        // current gauss layer of images

        blurred1 = gauss_7x7(blurred1, "img1_layer_" + layer_str);
        blurred2 = gauss_7x7(blurred2, "img1_layer_" + layer_str);

        // current gauss layer of masks

        mask1 = gauss_7x7(mask1, "mask1_layer_" + layer_str);
        mask2 = gauss_7x7(mask2, "mask2_layer_" + layer_str);
    }

    // add the highest pyramid layer (lowest frequency band)

    accumulator(x, y) += i32(blurred1(x,y) * mask1(x, y)) + i32(blurred2(x,y) * mask2(x, y));

    output(x,y) = u16_sat(accumulator(x,y));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    init_mask1.compute_root().parallel(y).vectorize(x, 16);

    accumulator.compute_root().parallel(y).vectorize(x, 16);

    for (int layer = 0; layer < num_layers; layer++) {

        accumulator.update(layer).parallel(y).vectorize(x, 16);
    }

    return output;
}

/*
 * brighten -- applies a specified gain to an image.
 */
Func brighten(Func input, float gain) {

    Func output("brighten_output");

    Var x, y;

    output(x, y) = u16_sat(gain * u32(input(x, y)));

    return output;
}

/*
 * tone_map -- iteratively compresses the dynamic range and boosts the gain
 * of the input. increases the compression and gain boost on each iteration
 * to reduce initial errors in both.
 */
Func tone_map(Func input, int width, int height, float comp, float gain) {

    Func grayscale("grayscale");
    Func normal_dist("luma_weight_distribution");
    Func output("tone_map_output");
    
    Var x, y, c, v;
    RDom r(0, 3);

    // distribution function (from exposure fusion paper)
    
    normal_dist(v) = f32(exp(-12.5f * pow(f32(v) / 65535.f - .5f, 2.f)));

    // use grayscale and brighter grayscale images for exposure fusion

    grayscale(x, y) = u16(sum(u32(input(x, y, r))) / 3);

    // gamma correct before combining

    Func bright("bright_grayscale");
    Func dark("dark_grayscale");
        
    dark = grayscale;

    int num_passes = 3;

    float gain_const = 1.f + gain / num_passes;
    float comp_const = 1.f + comp / num_passes;

    float gain_slope = (gain - gain_const) / (num_passes - 1);
    float comp_slope = (comp - comp_const) / (num_passes - 1);

    for (int pass = 0; pass < num_passes; pass++) {

        float norm_gain = pass * gain_slope + gain_const;
        float norm_comp = pass * comp_slope + comp_const;

        bright = brighten(dark, norm_comp);

        Func dark_gamma = gamma_correct(dark);
        Func bright_gamma = gamma_correct(bright);

        dark_gamma = combine(dark_gamma, bright_gamma, width, height, normal_dist);

        dark = brighten(gamma_inverse(dark_gamma), norm_gain);
    }

    // reintroduce image color

    output(x, y, c) = u16_sat(u32(input(x, y, c)) * u32(dark(x, y)) / max(1, grayscale(x, y)));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////
    
    grayscale.compute_root().parallel(y).vectorize(x, 16);
    
    normal_dist.compute_root().vectorize(v, 16);

    return output;
}

/*
 * median_filter -- applies a 2x2 median_filter to an image. Used as a helper for denoising via median-of-medians
 */
Func median_filter(Func input, int width, int height, bool is_flipped) {

    Func output("bilateral_filter_output");

    Var x, y, c;
    RDom r(-1, 2, -1, 2);

    output(x, y, c) = input(x, y, c);

    if (is_flipped) {
        output(x, y, 1) = (sum(input(x - r.x, y - r.y, 1)) - maximum(input(x - r.x, y - r.y, 1)) - minimum(input(x - r.x, y - r.y, 1))) / 2;
        output(x, y, 2) = (sum(input(x - r.x, y - r.y, 2)) - maximum(input(x - r.x, y - r.y, 2)) - minimum(input(x - r.x, y - r.y, 2))) / 2;
    }
    else {
        output(x, y, 1) = (sum(input(x + r.x, y + r.y, 1)) - maximum(input(x + r.x, y + r.y, 1)) - minimum(input(x + r.x, y + r.y, 1))) / 2;
        output(x, y, 2) = (sum(input(x + r.x, y + r.y, 2)) - maximum(input(x + r.x, y + r.y, 2)) - minimum(input(x + r.x, y + r.y, 2))) / 2;
    }

    output.compute_root().parallel(y).vectorize(x, 16);

    output.update(0).parallel(y).vectorize(x, 16);
    output.update(1).parallel(y).vectorize(x, 16);

    return output;
}


/*
 * bilateral_filter -- applies a bilateral filter to the UV channels of a YUV image to reduce chromatic noise.
 */
Func bilateral_filter(Func input, int width, int height) {
    
    Func k("gauss_kernel");
    Func weights("bilateral_weights");
    Func total_weights("bilateral_total_weights");
    Func bilateral("bilateral");
    Func output("bilateral_filter_output");

    Var x, y, dx, dy, c;

    // gaussian kernel

    k(dx, dy) = f32(0.f);

    k(-3, -3) = 0.007507f; k(-2, -3) = 0.011815f; k(-1, -3) = 0.015509f; k(0, -3) = 0.016982f; k(1, -3) = 0.015509f; k(2, -3) = 0.011815f; k(3, -3) = 0.007507f;
    k(-3, -2) = 0.011815f; k(-2, -2) = 0.018594f; k(-1, -2) = 0.024408f; k(0, -2) = 0.026726f; k(1, -2) = 0.024408f; k(2, -2) = 0.018594f; k(3, -2) = 0.011815f;
    k(-3, -1) = 0.015509f; k(-2, -1) = 0.024408f; k(-1, -1) = 0.032041f; k(0, -1) = 0.035083f; k(1, -1) = 0.032041f; k(2, -1) = 0.024408f; k(3, -1) = 0.015509f;
    k(-3,  0) = 0.016982f; k(-2,  0) = 0.026726f; k(-1,  0) = 0.035083f; k(0,  0) = 0.038414f; k(1,  0) = 0.035083f; k(2,  0) = 0.026726f; k(3,  0) = 0.016982f;
    k(-3,  1) = 0.015509f; k(-2,  1) = 0.024408f; k(-1,  1) = 0.032041f; k(0,  1) = 0.035083f; k(1,  1) = 0.032041f; k(2,  1) = 0.024408f; k(3,  1) = 0.015509f;
    k(-3,  2) = 0.011815f; k(-2,  2) = 0.018594f; k(-1,  2) = 0.024408f; k(0,  2) = 0.026726f; k(1,  2) = 0.024408f; k(2,  2) = 0.018594f; k(3,  2) = 0.011815f;
    k(-3,  3) = 0.007507f; k(-2,  3) = 0.011815f; k(-1,  3) = 0.015509f; k(0,  3) = 0.016982f; k(1,  3) = 0.015509f; k(2,  3) = 0.011815f; k(3,  3) = 0.007507f;

    RDom r(-3, 7, -3, 7);

    Func input_mirror = BoundaryConditions::mirror_interior(input, 0, width, 0, height);
    Func blur_input = gauss_7x7(input, "bilateral_prefilter");

    Expr dist = f32(i32(blur_input(x, y, c)) - i32(blur_input(x + dx, y + dy, c)));

    float sig2 = 300.f;
    Expr score = exp(-dist * dist / sig2);

    weights(dx, dy, x, y, c) = k(dx, dy) * score;

    total_weights(x, y, c) = sum(weights(r.x, r.y, x, y, c));

    bilateral(x, y, c) = sum(input_mirror(x + r.x, y + r.y, c) * weights(r.x, r.y, x, y, c)) / total_weights(x, y, c);

    output(x, y, c) = input(x, y, c);
    output(x, y, 1) = bilateral(x, y, 1);
    output(x, y, 2) = bilateral(x, y, 2);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    k.parallel(dy).parallel(dx).compute_root();

    weights.compute_at(output, y).vectorize(dx, 7);

    output.compute_root().parallel(y).vectorize(x, 16);

    output.update(0).parallel(y).vectorize(x, 16);
    output.update(1).parallel(y).vectorize(x, 16);

    return output;
}

/*
 * chroma_denoise -- reduces chromatic noise in an image through a combination of bilateral and median filtering.
 */
Func chroma_denoise(Func input, int width, int height, int num_passes) {
    
    Func output = rgb_to_yuv(input);

    for (int pass = 0; pass < num_passes; pass++) {

        output = median_filter(output, width, height, false);
        output = median_filter(output, width, height, true);
    }

    for (int pass = 0; pass < num_passes; pass++) {

        output = bilateral_filter(output, width, height);
    }

    return yuv_to_rgb(output);
}

/*
 * srgb -- converts the linear rgb color profile to the sRGB color gamut.
 */
Func srgb(Func input) {
    
    Func srgb_matrix("srgb_matrix");
    Func output("srgb_output");

    Var x, y, c;
    RDom r(0, 3);               // reduction over color channels

    // srgb conversion matrix; values taken from dcraw sRGB profile conversion

    srgb_matrix(x, y) = 0.f;

    srgb_matrix(0, 0) =  1.964399f; srgb_matrix(1, 0) = -1.119710f; srgb_matrix(2, 0) =  0.155311f;
    srgb_matrix(0, 1) = -0.241156f; srgb_matrix(1, 1) =  1.673722f; srgb_matrix(2, 1) = -0.432566f;
    srgb_matrix(0, 2) =  0.013887f; srgb_matrix(1, 2) = -0.549820f; srgb_matrix(2, 2) =  1.535933f;

    // resulting (linear) srgb image

    output(x, y, c) = u16_sat(sum(srgb_matrix(r, c) * input(x, y, r)));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    srgb_matrix.compute_root().parallel(y).parallel(x);

    return output;
}

/*
 * contrast -- boosts the contrast of an image following a scaled cosine curve.
 * Increasing the scale stretches the curve horizontally and decreases the 
 * contrast boost
 */
Func contrast(Func input, float scale) {

    Func output("contrast_output");

    Var x, y, c;

    float constant = 3.141592f / (2.f * scale);
    float sin_const = sin(constant);

    float x_factor = 3.141592f / (scale * 65535.f);

    float a = 65535.f / (2.f * sin_const);
    float b = a * sin_const;

    Expr x_scaled = f32(input(x,y,c)) * x_factor;

    output(x, y, c) = u16_sat(a * sin(x_scaled - constant) + b);


    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

/*
 * sharpen -- sharpens the image using difference of gaussian unsharp masking.
 */
Func sharpen(Func input) {

    Func output("sharpen_output");

    Var x, y, c;

    Func large_blurred;
    Func small_blurred;
    Func difference_of_gauss;

    small_blurred = gauss_7x7(input, "unsharp_small_blur");
    large_blurred = gauss_7x7(small_blurred, "unsharp_large_blur");
    difference_of_gauss = diff(small_blurred, large_blurred, "unsharp_DoG");
    output(x, y, c) = u16(clamp(i32(input(x, y, c)) + difference_of_gauss(x, y, c), 0, 65535));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////
    
    //output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

/*
 * interleaves the color channels to that the image can be written to a PNG
 */
Func u8bit_interleaved(Func input) {

    Func output("8bit_interleaved_output");

    Var c, x, y;

    // Convert to 8 bit
    
    output(c, x, y) = u8_sat(input(x, y, c) / 256);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

/*
 * finish -- Applies a series of standard local and global operations to a merged image to produce an attractive output with
 * minimal user input. Takes advantage of noise-reduction through frame-merging to make dark details more distinguishable without
 * blowing out highlights or introducing noise.
 */
Func finish(Func input, int width, int height, const BlackPoint bp, const WhitePoint wp, const WhiteBalance &wb, const Compression c, const Gain g) {

    // 1. Black-layer subtraction and white-layer scaling
    Func black_white_point_output = black_white_point(Func(input), bp, wp);

    // 2. White balancing
    Func white_balance_output = white_balance(black_white_point_output, width, height, wb);

    // 3. Demosaicking
    Func demosaic_output = demosaic(white_balance_output, width, height);

    // 4. Chroma denoising
    Func chroma_denoised_output = chroma_denoise(demosaic_output, width, height, 2);

    // 5. sRGB color correction
    Func srgb_output = srgb(chroma_denoised_output);
    
    // 6. Tone mapping
    Func tone_map_output = tone_map(srgb_output, width, height, c, g);

    // 7. Gamma correction
    Func gamma_correct_output = gamma_correct(tone_map_output);

    // 8. Global contrast increase
    Func contrast_output = contrast(gamma_correct_output, 1.f);

    // 9. Sharpening
    Func sharpen_output = sharpen(contrast_output);

    // 10. Convert to 8 bit interleaved image
    return u8bit_interleaved(sharpen_output);
}