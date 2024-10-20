#include "finish.h"

#include "Halide.h"
#include "util.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

/*
 * black_white_level -- Renormalizes an image based on input black and white
 * levels to take advantage of the full 16-bit integer depth. This is a
 * necessary step for camera white balance levels to be valid.
 */
Func black_white_level(Func input, const Expr bp, const Expr wp) {

    Func output("black_white_level_output");

    Var x, y;

    Expr white_factor = 65535.f / (wp - bp);

    output(x, y) = u16_sat((i32(input(x, y)) - bp) * white_factor);

    return output;
}

/*
 * white_balance -- Corrects white-balance of a mosaicked image based on input
 * color multipliers. Note that the two green channels in the bayer pattern
 * are white-balanced separately.
 */
Func white_balance(Func input, Expr width, Expr height, const CompiletimeWhiteBalance &wb) {

    Func output("white_balance_output");

    Var x, y;
    RDom r(0, width / 2, 0, height / 2);

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
 * demosaic -- Interpolates color channels in the bayer mosaic based on the
 * work of Malvar et al. Assumes that data is laid out in an RG/GB pattern.
 * https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/Demosaicing_ICASSP04.pdf
 */
Func demosaic(Func input, Expr width, Expr height) {

    Buffer<int32_t> f0(5, 5, "demosaic_f0");             // G at R locations; G at B locations
    Buffer<int32_t> f1(5, 5, "demosaic_f1");             // R at green in R row, B column; B at green in B row, R column
    Buffer<int32_t> f2(5, 5, "demosaic_f2");             // R at green in B row, R column; B at green in R row, B column
    Buffer<int32_t> f3(5, 5, "demosaic_f3");             // R at blue in B row, B column; B at red in R row, R column

    f0.translate({-2, -2});
    f1.translate({-2, -2});
    f2.translate({-2, -2});
    f3.translate({-2, -2});

    Func d0("demosaic_0");
    Func d1("demosaic_1");
    Func d2("demosaic_2");
    Func d3("demosaic_3");

    Func output("demosaic_output");

    Var x, y, c;
    RDom r0(-2, 5, -2, 5);
    RDom r1(0, width / 2, 0, height / 2);

    // mirror input image with overlapping edges to keep mosaic pattern consistency

    Func input_mirror = BoundaryConditions::mirror_interior(input, {Range(0, width), Range(0, height)});

    // demosaic filters

    f0.fill(0);
    f1.fill(0);
    f2.fill(0);
    f3.fill(0);

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
    f3(-2, 0) = -3;                 f3(0,  0) = 12;                 f3(2, 0) = -3;
                    f3(-1, 1) = 4;                  f3(1,  1) = 4;
                                    f3(0,  2) = -3;

    // intermediate demosaic functions

    d0(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f0(r0.x, r0.y)) / f0_sum);
    d1(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f1(r0.x, r0.y)) / f1_sum);
    d2(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f2(r0.x, r0.y)) / f2_sum);
    d3(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f3(r0.x, r0.y)) / f3_sum);

    // resulting demosaicked function

    Expr R_row = y % 2 == 0;
    Expr B_row = !R_row;
    Expr R_col = x % 2 == 0;
    Expr B_col = !R_col;
    Expr at_R = c == 0;
    Expr at_G = c == 1;
    Expr at_B = c == 2;

    output(x, y, c) = select(at_R && R_row && B_col, d1(x, y),
                             at_R && B_row && R_col, d2(x, y),
                             at_R && B_row && B_col, d3(x, y),
                             at_G && R_row && R_col, d0(x, y),
                             at_G && B_row && B_col, d0(x, y),
                             at_B && B_row && R_col, d1(x, y),
                             at_B && R_row && B_col, d2(x, y),
                             at_B && R_row && R_col, d3(x, y),
                             input(x, y));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////
    d0.compute_root().parallel(y).vectorize(x, 16);
    d1.compute_root().parallel(y).vectorize(x, 16);
    d2.compute_root().parallel(y).vectorize(x, 16);
    d3.compute_root().parallel(y).vectorize(x, 16);

    output.compute_root().parallel(y)
        .align_bounds(x, 2).unroll(x, 2)
        .align_bounds(y, 2).unroll(y, 2)
        .vectorize(x, 16);
    return output;
}

/*
 * bilateral_filter -- Applies a 7x7 bilateral filter to the UV channels of a
 * YUV input to reduce chromatic noise. Chroma values above a threshold are
 * weighted as 0 to decrease amplification of saturation artifacts, which can
 * occur around bright highlights.
 */
Func bilateral_filter(Func input, Expr width, Expr height) {

    Buffer<float> k(7, 7, "gauss_kernel");
    k.translate({-3, -3});

    Func weights("bilateral_weights");
    Func total_weights("bilateral_total_weights");
    Func bilateral("bilateral");
    Func output("bilateral_filter_output");

    Var x, y, dx, dy, c;
    RDom r(-3, 7, -3, 7);

    // gaussian kernel

    k.fill(0.f);
    k(-3, -3) = 0.000690f; k(-2, -3) = 0.002646f; k(-1, -3) = 0.005923f; k(0, -3) = 0.007748f; k(1, -3) = 0.005923f; k(2, -3) = 0.002646f; k(3, -3) = 0.000690f;
    k(-3, -2) = 0.002646f; k(-2, -2) = 0.010149f; k(-1, -2) = 0.022718f; k(0, -2) = 0.029715f; k(1, -2) = 0.022718f; k(2, -2) = 0.010149f; k(3, -2) = 0.002646f;
    k(-3, -1) = 0.005923f; k(-2, -1) = 0.022718f; k(-1, -1) = 0.050855f; k(0, -1) = 0.066517f; k(1, -1) = 0.050855f; k(2, -1) = 0.022718f; k(3, -1) = 0.005923f;
    k(-3,  0) = 0.007748f; k(-2,  0) = 0.029715f; k(-1,  0) = 0.066517f; k(0,  0) = 0.087001f; k(1,  0) = 0.066517f; k(2,  0) = 0.029715f; k(3,  0) = 0.007748f;
    k(-3,  1) = 0.005923f; k(-2,  1) = 0.022718f; k(-1,  1) = 0.050855f; k(0,  1) = 0.066517f; k(1,  1) = 0.050855f; k(2,  1) = 0.022718f; k(3,  1) = 0.005923f;
    k(-3,  2) = 0.002646f; k(-2,  2) = 0.010149f; k(-1,  2) = 0.022718f; k(0,  2) = 0.029715f; k(1,  2) = 0.022718f; k(2,  2) = 0.010149f; k(3,  2) = 0.002646f;
    k(-3,  3) = 0.000690f; k(-2,  3) = 0.002646f; k(-1,  3) = 0.005923f; k(0,  3) = 0.007748f; k(1,  3) = 0.005923f; k(2,  3) = 0.002646f; k(3,  3) = 0.000690f;

    Func input_mirror = BoundaryConditions::mirror_interior(input, {Range(0, width), Range(0, height)});

    Expr dist = f32(i32(input_mirror(x, y, c)) - i32(input_mirror(x + dx, y + dy, c)));

    float sig2 = 100.f;                 // 2 * sigma ^ 2

    // score represents the weight contribution due to intensity difference

    float threshold = 25000.f;

    Expr score = select(abs(input_mirror(x + dx, y + dy, c)) > threshold, 0.f, exp(-dist * dist / sig2));

    // combine score with gaussian weights and compute total weights in search region

    weights(dx, dy, x, y, c) = k(dx, dy) * score;

    total_weights(x, y, c) = sum(weights(r.x, r.y, x, y, c));

    // output normalizes weights to total weights

    bilateral(x, y, c) = sum(input_mirror(x + r.x, y + r.y, c) * weights(r.x, r.y, x, y, c)) / total_weights(x, y, c);

    output(x, y, c) = f32(input(x, y, c));

    output(x, y, 1) = bilateral(x, y, 1);
    output(x, y, 2) = bilateral(x, y, 2);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    //k.parallel(dy).parallel(dx).compute_root();

    weights.compute_at(output, y).vectorize(x, 16);

    output.compute_root().parallel(y).vectorize(x, 16);

    output.update(0).parallel(y).vectorize(x, 16);
    output.update(1).parallel(y).vectorize(x, 16);

    return output;
}

/*
 * desaturate_noise -- Reduces chromatic noise by blurring UV channels of a YUV
 * input in and using the result only if it falls within constraints on by what
 * factor and absolute threshold the chroma magnitudes fall.
 */
Func desaturate_noise(Func input, Expr width, Expr height) {

    Func output("desaturate_noise_output");

    Var x, y, c;

    Func input_mirror = BoundaryConditions::mirror_image(input, {Range(0, width), Range(0, height)});

    Func blur = gauss_15x15(gauss_15x15(input_mirror, "desaturate_noise_blur1"), "desaturate_noise_blur2");

    // magnitude of chroma channel can increase by at most the factor

    float factor = 1.4f;

    // denoise will only be applied when input and output value are less than threshold

    float threshold = 25000.f;

    output(x, y, c) = input(x, y, c);

    output(x, y, 1) = select((abs(blur(x, y, 1)) / abs(input(x, y, 1)) < factor) &&
                             (abs(input(x, y, 1)) < threshold) && (abs(blur(x, y, 1)) < threshold),
                             .7f * blur(x, y, 1) + .3f * input(x, y, 1), input(x, y, 1));

    output(x, y, 2) = select((abs(blur(x, y, 2)) / abs(input(x, y, 2)) < factor) &&
                             (abs(input(x, y, 2)) < threshold) && (abs(blur(x, y, 2)) < threshold),
                             .7f * blur(x, y, 2) + .3f * input(x, y, 2), input(x, y, 2));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

/*
 * increase_saturation -- Increases magnitude of UV channels for YUV input.
 */
Func increase_saturation(Func input, float strength) {

    Func output("increase_saturation_output");

    Var x, y, c;

    output(x, y, c) = strength * input(x, y, c);
    output(x, y, 0) = input(x, y, 0);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

/*
 * chroma_denoise -- Reduces chromatic noise in an image through a combination
 * bilateral filtering and shadow desaturation. The noise removal algorithms
 * will be applied iteratively in order of increasing aggressiveness, with the
 * total number of passes determined by input.
 */
Func chroma_denoise(Func input, Expr width, Expr height, int num_passes) {

    Func output = rgb_to_yuv(input);

    int pass = 0;

    if (num_passes > 0) output = bilateral_filter(output, width, height);
    pass++;

    while(pass < num_passes) {

        output = desaturate_noise(output, width, height);
        pass++;
    }

    if (num_passes > 2) output = increase_saturation(output, 1.1f);

    return yuv_to_rgb(output);
}

/*
 * combine -- Combines two greyscale inputs with a laplacian pyramid by using
 * the input distribution function to weight inputs relative to each other.
 * This technique is a modified version of the exposure fusion method described
 * by Mertens et al.
 * http://ntp-0.cs.ucl.ac.uk/staff/j.kautz/publications/exposure_fusion.pdf
 */
Func combine(Func im1, Func im2, Expr width, Expr height, Func dist) {

    Func init_mask1("mask1_layer_0");
    Func init_mask2("mask2_layer_0");
    Func accumulator("combine_accumulator");
    Func output("combine_output");

    Var x, y;

    // mirror input images

    Func im1_mirror = BoundaryConditions::repeat_edge(im1, {Range(0, width), Range(0, height)});
    Func im2_mirror = BoundaryConditions::repeat_edge(im2, {Range(0, width), Range(0, height)});

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

    int num_layers = 2;

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

        blurred1 = gauss_7x7(blurred1, "img1_layer_" + layer_str + "_1");
        blurred2 = gauss_7x7(blurred2, "img1_layer_" + layer_str + "_2");

        // current gauss layer of masks

        mask1 = gauss_7x7(mask1, "mask1_layer_" + layer_str + "_1");
        mask2 = gauss_7x7(mask2, "mask2_layer_" + layer_str + "_2");
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
 * brighten -- Applies a specified gain to an input.
 */
Func brighten(Func input, Expr gain) {

    Func output("brighten_output");

    Var x, y;

    output(x, y) = u16_sat(gain * u32(input(x, y)));

    return output;
}

/*
 * tone_map -- Iteratively compresses the dynamic range and boosts the gain
 * of the input. Compression and gain are determined by input and are applied
 * with an increasing strength in each iteration to ensure a natural looking
 * dynamic range compression.
 */
Func tone_map(Func input, Expr width, Expr height, Expr comp, Expr gain) {

    Func normal_dist("luma_weight_distribution");
    Func grayscale("grayscale");
    Func output("tone_map_output");

    Var x, y, c, v;
    RDom r(0, 3);

    // distribution function (from exposure fusion paper)

    normal_dist(v) = f32(exp(-12.5f * pow(f32(v) / 65535.f - .5f, 2.f)));

    // use grayscale and brighter grayscale images for exposure fusion

    grayscale(x, y) = u16(sum(u32(input(x, y, r))) / 3);

    Func bright, dark;

    dark = grayscale;

    // more passes and smaller compression and gain values produces more natural results

    int num_passes = 3;

    // constants used to determine compression and gain values at each iteration

    Expr comp_const = 1.f + comp / num_passes;
    Expr gain_const = 1.f + gain / num_passes;

    Expr comp_slope = (comp - comp_const) / (num_passes - 1);
    Expr gain_slope = (gain - gain_const) / (num_passes - 1);

    for (int pass = 0; pass < num_passes; pass++) {

        // compute compression and gain at given iteration

        Expr norm_comp = pass * comp_slope + comp_const;
        Expr norm_gain = pass * gain_slope + gain_const;

        bright = brighten(dark, norm_comp);

        // gamma correct before fusion

        Func dark_gamma = gamma_correct(dark);
        Func bright_gamma = gamma_correct(bright);

        dark_gamma = combine(dark_gamma, bright_gamma, width, height, normal_dist);

        // invert gamma correction and apply gain

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
 * srgb -- Converts to linear sRGB color profile. Conversion values taken from
 * dcraw sRGB profile conversion.
 * https://www.cybercom.net/~dcoffin/dcraw/
 */
Func srgb(Func input, Func srgb_matrix) {
    Func output("srgb_output");

    Var x, y, c;
    RDom r(0, 3);

//    Buffer<float> srgb_matrix(3, 3, "srgb_matrix");
//    original hardcoded values, just for reference:
//    srgb_matrix(0, 0) =  1.964399f; srgb_matrix(1, 0) = -1.119710f; srgb_matrix(2, 0) =  0.155311f;
//    srgb_matrix(0, 1) = -0.241156f; srgb_matrix(1, 1) =  1.673722f; srgb_matrix(2, 1) = -0.432566f;
//    srgb_matrix(0, 2) =  0.013887f; srgb_matrix(1, 2) = -0.549820f; srgb_matrix(2, 2) =  1.535933f;

    // resulting (linear) srgb image
    output(x, y, c) = u16_sat(sum(srgb_matrix(r, c) * input(x, y, r)));

    return output;
}

/*
 * contrast -- Boosts the global contrast of an image with an S-shaped
 * scaled cosine curve followed by black level subtraction and renormalization.
 */
Func contrast(Func input, float strength, int black_level) {

    Func output("contrast_output");

    Var x, y, c;

    // scale stretches the curve horizontally, decreasing the amount of contrast

    float scale = 0.8f + 0.3f / std::min(1.f, strength);

    // constants for scaling cosine curve to the domain/range of image values

    float inner_constant = 3.141592f / (2.f * scale);
    float sin_constant = sin(inner_constant);

    float slope = 65535.f / (2.f * sin_constant);
    float constant = slope * sin_constant;

    float factor = 3.141592f / (scale * 65535.f);

    Expr val = factor * f32(input(x, y, c));

    // scaled cosine output produces S-shaped map over image values

    output(x, y, c) = u16_sat(slope * sin(val - inner_constant) + constant);

    // subtract black level and scale

    float white_scale = 65535.f / (65535.f - black_level);

    output(x, y, c) = u16_sat((i32(output(x, y, c)) - black_level) * white_scale);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

/*
 * sharpen -- Sharpens input using difference of Gaussian unsharp masking
 * applied only to the image luminance so as to not amplify chroma noise.
 */
Func sharpen(Func input, float strength) {

    Func output_yuv("sharpen_output_yuv");

    Var x, y, c;

    // convert to yuv

    Func yuv_input = rgb_to_yuv(input);

    // apply two gaussian passes

    Func small_blurred = gauss_7x7(yuv_input, "unsharp_small_blur");
    Func large_blurred = gauss_7x7(small_blurred, "unsharp_large_blur");

    // add difference of gaussians to Y channel

    Func difference_of_gauss = diff(small_blurred, large_blurred, "unsharp_DoG");

    output_yuv(x, y, c) = yuv_input(x, y, c);
    output_yuv(x, y, 0) = yuv_input(x, y, 0) + strength * difference_of_gauss(x, y, 0);

    // convert back to rgb

    Func output = yuv_to_rgb(output_yuv);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output_yuv.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

/*
 * u8bit_interleaved -- Converts to 8 bits and interleaves color channels so
 * output can be easily written to an output file.
 */
Func u8bit_interleaved(Func input) {

    Func output("_8bit_interleaved_output");

    Var c, x, y;

    // Convert to 8 bit

    output(c, x, y) = u8_sat(input(x, y, c) / 256);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

Halide::Func shift_bayer_to_rggb(Halide::Func input, const Halide::Expr cfa_pattern) {
    Func output("rggb_input");
    Var x, y, c;
    output(x, y) = select(
        cfa_pattern == int(CfaPattern::CFA_RGGB), input(x, y),
        cfa_pattern == int(CfaPattern::CFA_GRBG), input(x + 1, y),
        cfa_pattern == int(CfaPattern::CFA_GBRG), input(x, y + 1),
        cfa_pattern == int(CfaPattern::CFA_BGGR), input(x + 1, y + 1),
        0);
    return output;
}

/*
 * finish -- Applies a series of standard local and global image processing
 * operations to an input mosaicked image, producing a pleasant color output.
 * Input pecifies black-level, white-level and white balance. Additionally,
 * tone mapping is applied to the image, as specified by the input compression
 * and gain amounts. This produces natural-looking brightened shadows, without
 * blowing out highlights. The output values are 8-bit.
 */
Halide::Func finish(Halide::Func input, Expr width, Expr height, Expr bp, Expr wp, const CompiletimeWhiteBalance &wb, const Expr cfa_pattern, Halide::Func ccm, const Expr c, const Expr g) {
    int denoise_passes = 1;
    float contrast_strength = 5.f;
    int black_level = 2000;
    float sharpen_strength = 2.f;

    Func bayer_shifted = shift_bayer_to_rggb(input, cfa_pattern);
    
    // 1. Black-level subtraction and white-level scaling
    Func black_white_level_output = black_white_level(bayer_shifted, bp, wp);

    // 2. White balancing

    Func white_balance_output = white_balance(black_white_level_output, width, height, wb);

    // 3. Demosaicking

    Func demosaic_output = demosaic(white_balance_output, width, height);

    // 4. Chroma denoising

    Func chroma_denoised_output = chroma_denoise(demosaic_output, width, height, denoise_passes);

    // 5. sRGB color correction

    Func srgb_output = srgb(demosaic_output, ccm);

    // 6. Tone mapping

    Func tone_map_output = tone_map(srgb_output, width, height, c, g);

    // 7. Gamma correction

    Func gamma_correct_output = gamma_correct(tone_map_output);

    // 8. Global contrast increase

    Func contrast_output = contrast(gamma_correct_output, contrast_strength, black_level);

    // 9. Sharpening

    Func sharpen_output = sharpen(contrast_output, sharpen_strength);

    return u8bit_interleaved(contrast_output);
}

Func finish(Func input, int width, int height, const BlackPoint bp, const WhitePoint wp, const WhiteBalance &wb, const CfaPattern cfa, Halide::Func ccm, const Compression c, const Gain g) {
    return finish(input, width, height, bp, wp, wb, cfa, ccm, c, g);
}
