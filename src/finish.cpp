#include "finish.h"

#include "Halide.h"
#include "halide_image_io.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

// Should black point be separately defined for RGB (or RGGB) channels?
Image<uint16_t> black_point(Image<uint16_t> input, const BlackPoint bp) {

    Func output("black_point_output");

    Var x, y;

    output(x, y) = u16_sat(i32(input(x, y)) - bp);

    // schedule

    //output.compute_root().parallel(y).vectorize(x, 16);

    // realize image

    Image<uint16_t> output_img(input.width(), input.height());

    output.realize(output_img);

    return output_img;
}


Image<uint16_t> white_balance(Image<uint16_t> input, const WhiteBalance &wb) {

    Func output("white_balance_output");

    Var x, y;

    output(x, y) = u16(0);

    RDom r(0, input.width() / 2, 0, input.height() / 2);

    output(r.x * 2    , r.y * 2    ) = u16_sat(wb.r  * f32(input(r.x * 2    , r.y * 2    )));   // red
    output(r.x * 2 + 1, r.y * 2    ) = u16_sat(wb.g0 * f32(input(r.x * 2 + 1, r.y * 2    )));   // green 0
    output(r.x * 2    , r.y * 2 + 1) = u16_sat(wb.g1 * f32(input(r.x * 2    , r.y * 2 + 1)));   // green 1
    output(r.x * 2 + 1, r.y * 2 + 1) = u16_sat(wb.b  * f32(input(r.x * 2 + 1, r.y * 2 + 1)));   // blue

    // schedule

    //output.compute_root().parallel(y).vectorize(x, 16);

    // realize image

    Image<uint16_t> output_img(input.width(), input.height());

    output.realize(output_img);

    return output_img;
}

Image<uint16_t> demosaic(Image<uint16_t> input) {

    // Technique from: https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/Demosaicing_ICASSP04.pdf

    // assumes RG/GB Bayer pattern

    Func f0("demosaic_f0");     // G at R locations; G at B locations
    Func f1("demosaic_f1");     // R at green in R row, B column; B at green in B row, R column
    Func f2("demosaic_f2");     // R at green in B row, R column; B at green in R row, B column
    Func f3("demosaic_f3");     // R at blue in B row, B column; B at red in R row, R column

    Var x, y;

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

    Func d0("demosaic_0");
    Func d1("demosaic_1");
    Func d2("demosaic_2");
    Func d3("demosaic_3");

    RDom r0(-2, 5, -2, 5);

    Func input_mirror = BoundaryConditions::mirror_image(input);

    d0(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f0(r0.x, r0.y)) / f0_sum);
    d1(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f1(r0.x, r0.y)) / f1_sum);
    d2(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f2(r0.x, r0.y)) / f2_sum);
    d3(x, y) = u16_sat(sum(i32(input_mirror(x + r0.x, y + r0.y)) * f3(r0.x, r0.y)) / f3_sum);

    Func output("demosaic_output");
    
    Var c;

    output(x, y, c) = input(x, y);                                              // initialize each channel to input mosaicked image

    RDom r1(0, input.width() / 2, 0, input.height() / 2); 
    
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

    // schedule

    f0.compute_root().parallel(y).vectorize(x, 16);
    f1.compute_root().parallel(y).vectorize(x, 16);
    f2.compute_root().parallel(y).vectorize(x, 16);
    f3.compute_root().parallel(y).vectorize(x, 16);

    d0.compute_root().parallel(y).vectorize(x, 16);
    d1.compute_root().parallel(y).vectorize(x, 16);
    d2.compute_root().parallel(y).vectorize(x, 16);
    d3.compute_root().parallel(y).vectorize(x, 16);

    // realize image

    Image<uint16_t> output_img(input.width(), input.height(), 3);

    output.realize(output_img);

    return output_img;
}


Image<uint8_t> finish(Image<uint16_t> input, const BlackPoint bp, const WhiteBalance &wb) {

    // 1. Black-level subtraction
    Image<uint16_t> black_point_output = black_point(input, bp);

    // 2. Lens shading correction
    // LIKELY OMIT

    // 3. White balancing
    Image<uint16_t> white_balance_output = white_balance(black_point_output, wb);

    // 4. Demosaicking
    Image<uint16_t> demosaic_output = demosaic(white_balance_output);

    // 5. Chroma denoising
    // LIKELY OMIT

    // 6. Color correction
    // TODO: add gamma correction here
    
    // 7. Dynamic range compression
    // TODO: tone mapping

    // 8. Dehazing
    // 9. Global tone adjustment
    // 10. Chromatic aberration correction
    // 11. Sharpening
    // 12. Hue-specific color adjustments
    // 13. Dithering
    // LIKELY OMIT


    // The output image must have an RGB interleaved memory layout
    Func output;
    Var x, y, c;

    //int brighten_factor = 80;   // for now, because gamma correction is not implemented
    output(c, x, y) = u8_sat(demosaic_output(x, y, c));

    Image<uint8_t> output_img(3, input.width(), input.height());

    output.realize(output_img);
    output_img.transpose(0, 1);
    output_img.transpose(1, 2);

    return output_img;
}