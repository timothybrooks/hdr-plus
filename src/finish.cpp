#include "finish.h"

#include "Halide.h"
#include "halide_image_io.h"
#include "util.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

Func black_white_point(Func input, const BlackPoint bp, const BlackPoint wp) {

    Func output("black_white_point_output");

    Var x, y;

    float white_factor = 65535.f / (wp - bp);

    output(x, y) = u16_sat((i32(input(x, y)) - bp) * white_factor);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}


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

Func demosaic(Func input, int width, int height) {

    // Technique from: https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/Demosaicing_ICASSP04.pdf

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

    f0.compute_root().parallel(y).vectorize(x, 16);
    f1.compute_root().parallel(y).vectorize(x, 16);
    f2.compute_root().parallel(y).vectorize(x, 16);
    f3.compute_root().parallel(y).vectorize(x, 16);

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

Func gamma_single_channel(Func input) {
    // http://www.color.org/sRGB.xalter
    // see formulas 1.2a and 1.2b

    Func output("gamma_correct_output");

    Var x, y, c;

    // constants for gamma correction

    int cutoff = 200;                   // ceil(0.00304 * UINT16_MAX)
    float gamma_toe = 12.92;
    float gamma_pow = 0.416667;         // 1 / 2.4
    float gamma_fac = 680.552897;       // 1.055 * UINT16_MAX ^ (1 - gamma_pow);
    float gamma_con = -3604.425;        // -0.055 * UINT16_MAX

    output(x, y) = u16(select(input(x, y) < cutoff,
                                 gamma_toe * input(x, y),
                                 gamma_fac * pow(input(x, y), gamma_pow) + gamma_con));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

Func gamma_inverse_single_channel(Func input) {

    Func output("gamma_inverse_output");

    Var x, y;

    // constants for gamma correction

    int cutoff = 2575;                   // ceil(1/0.00304 * UINT16_MAX)
    float gamma_toe = 0.0774;            // 1 / 12.92
    float gamma_pow = 2.4;
    float gamma_fac = 57632.49226;       // 1 / 1.055 ^ gamma_pow * U_INT16_MAX;
    float gamma_offset = .055;

    output(x, y) = u16(select(input(x, y) < cutoff,
                              gamma_toe * input(x, y),
                              pow(f32(input(x, y))/65535.f + gamma_offset, gamma_pow) * gamma_fac));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

Func get_weights(Func im1, Func im2, Func dist) {
    Func output("weights_1");
    Var x, y;
    Expr g1 = f64(dist(im1(x,y)));
    Expr g2 = f64(dist(im2(x,y)));
    output(x, y) = g1 / (g1 + g2);
    return output;
}

Func invert_weights(Func weights) {
    Func output("weights_2");
    Var x, y;
    output(x,y) = 1.f - weights(x,y);
    return output;
}

Func diff(Func im, Func im2) {
    Func output(im.name() + "_laplace");
    Var x, y;
    output(x,y) = i32(im(x,y)) - i32(im2(x,y));
    return output;
}

//exposure fusion as described by Mertens et al. Modified to only use intensity
//http://ntp-0.cs.ucl.ac.uk/staff/j.kautz/publications/exposure_fusion.pdf
Func combine(Func im1, Func im2, Func dist) {
    Var x, y;

    Func result("combine_intermediate_output");
    
    result(x,y) = i32(0);
    Func unblurred1 = im1;
    Func unblurred2 = im2;
    Func blurred1 = gauss_7x7(im1, false);
    Func blurred2 = gauss_7x7(im2, false);
    Func laplace1 = diff(unblurred1, blurred1);
    Func laplace2 = diff(unblurred2, blurred2);
    Func mask1 = get_weights(im1, im2, dist);
    Func mask2 = invert_weights(mask1);
    for (int i = 0; i < 5; i++) {
        result(x, y) += i32(laplace1(x,y) * mask1(x,y));
        unblurred1 = blurred1;
        blurred1 = gauss_7x7(blurred1, false);
        laplace1 = diff(unblurred1, blurred1);
        mask1 = gauss_7x7(mask1, true);
        result(x, y) += i32(laplace2(x,y) * mask2(x,y));
        unblurred2 = blurred2;
        blurred2 = gauss_7x7(blurred2, false);
        laplace2 = diff(unblurred2, blurred2);
        mask2 = gauss_7x7(mask2, true);
    }
    result(x, y) += i32(blurred1(x,y) * mask1(x, y));
    result(x, y) += i32(blurred2(x,y) * mask2(x, y));

    Func output("combine_output");
    output(x,y) = u16_sat(result(x,y));
    return output;
}

Func tone_map(Func input, int width, int height) {
    Func input_repeat = BoundaryConditions::repeat_edge(input, 0 , width, 0, height);
    int num_channels = 3;
    RDom r(0, num_channels);

    Var x, y;

    Func grayscale("grayscale");
    grayscale(x, y) = u16(sum(u32(input_repeat(x, y, r))) / num_channels);

    Func brighter("brighter_grayscale");
    brighter(x, y) = u16_sat(3 * u32(grayscale(x, y)));

    Func gamma_grayscale = gamma_single_channel(grayscale);
    Func gamma_brighter = gamma_single_channel(brighter);

    //distribution function from exposure fusion paper () 
    Func dist("luma_weight_distribution");
    dist(x) = f64(exp(-12.5f * pow(f64(x)/65535.f - .5f, 2.f)));

    //Func result = combine(test_gradient, brighter, dist);
    Func result = gamma_inverse_single_channel(combine(gamma_grayscale, gamma_brighter, dist));

    Func output("tone_map_output");
    Var c;
    // output(x,y,c) = u16_sat(result(x,y) + 256);
    output(x,y,c) = u16_sat(u32(input(x, y, c)) * u32(result(x, y)) / max(1, grayscale(x, y)));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    //TODO

    return output;
}

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

    srgb_matrix.compute_root().parallel(y).vectorize(x, 16);;

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

Func gamma_correct(Func input) {

    // http://www.color.org/sRGB.xalter
    // see formulas 1.2a and 1.2b

    Func output("gamma_correct_output");

    Var x, y, c;

    // constants for gamma correction

    int cutoff = 200;                   // ceil(0.00304 * UINT16_MAX)
    float gamma_toe = 12.92;
    float gamma_pow = 0.416667;         // 1 / 2.4
    float gamma_fac = 680.552897;       // 1.055 * UINT16_MAX ^ (1 - gamma_pow);
    float gamma_con = -3604.425;        // -0.055 * UINT16_MAX

    output(x, y, c) = u16(select(input(x, y, c) < cutoff,
                                 gamma_toe * input(x, y, c),
                                 gamma_fac * pow(input(x, y, c), gamma_pow) + gamma_con));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

Image<uint8_t> u8bit_interleaved(Func input, int width, int height) {

    Func output("8bit_interleaved_output");

    Var c, x, y;

    // Convert to 8 bit

    output(c, x, y) = u8_sat(input(x, y, c) / 256);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    ///////////////////////////////////////////////////////////////////////////
    // realize image
    ///////////////////////////////////////////////////////////////////////////

    Image<uint8_t> output_img(3, width, height);

    output.realize(output_img);

    // transpose to account for interleaved layout

    output_img.transpose(0, 1);
    output_img.transpose(1, 2);

    return output_img;
}

Image<uint8_t> finish(Image<uint16_t> input, const BlackPoint bp, const WhitePoint wp, const WhiteBalance &wb) {

    int width = input.width();
    int height = input.height();

    // 1. Black-level subtraction and white-level scaling
    Func black_white_point_output = black_white_point(Func(input), bp, wp);

    // 2. White balancing
    Func white_balance_output = white_balance(black_white_point_output, width, height, wb);

    // 3. Demosaicking
    Func demosaic_output = demosaic(white_balance_output, width, height);

    // 4. Chroma denoising
    // TODO

    // 5. sRGB color correction
    Func srgb_output = srgb(demosaic_output);
    
    // 6. Tone mapping
    Func tone_map_output = tone_map(srgb_output, width, height);

    // 7. Gamma correction
    Func gamma_correct_output = gamma_correct(tone_map_output);

    // 8. Global contrast increase
    // TODO

    // 9. Sharpening
    // TODO

    // 10. Convert to 8 bit interleaved image
    return u8bit_interleaved(gamma_correct_output, width, height);    
}