#include "Halide.h"
#include "halide_image_io.h"

using namespace Halide;

Image<uint16_t> demosaic(Image<uint16_t> input) {

    // Technique from:
    // https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/Demosaicing_ICASSP04.pdf

    // Assumes RG/GB Bayer pattern

    Func f0("demosaic_f0"); // G at R locations; G at B locations
    Func f1("demosaic_f1"); // R at green in R row, B column; B at green in B row, R column
    Func f2("demosaic_f2"); // R at green in B row, R column; B at green in R row, B column
    Func f3("demosaic_f3"); // R at blue in B row, B column; B at red in R row, R column

    Var x, y;

    f0(x,y) = 0;
    f1(x,y) = 0;
    f2(x,y) = 0;
    f3(x,y) = 0;

                                    f0(0, -2) = -1;
                                    f0(0, -1) =  2;
    f0(-2, 0) = -1; f0(-1, 0) = 2;  f0(0,  0) =  4; f0(1, 0) = 2;   f0(2, 0) = -1;
                                    f0(0,  1) =  2;
                                    f0(0,  2) = -1;

    // Filter results must be divided by 2
                                    f1(0, -2) =  1;
                    f1(-1,-1) = -2;                 f1(1, -1) = -2;
    f1(-2, 0) = -2; f1(-1, 0) =  8; f1(0,  0) = 10; f1(1,  0) =  8; f1(2, 0) = -2;
                    f1(-1, 1) = -2;                 f1(1,  1) = -2;
                                    f1(0,  2) =  1;

    // Filter results must be divided by 2
                                    f2(0, -2) = -2;
                    f2(-1,-1) = -2; f2(0, -1) =  8; f2(1, -1) = -2;
    f2(-2, 0) = 1;                  f2(0,  0) = 10;                 f2(2, 0) = 1;
                    f2(-1, 1) = -2; f2(0,  1) =  8; f2(1,  1) = -2;
                                    f2(0,  2) = -2;

    // Filter results must be divided by 2
                                    f3(0, -2) = -3;
                    f3(-1,-1) = 4;                  f3(1, -1) = 4;
    f3(-2, 0) = -3;                 f3(0,  0) = 12;                 f3(2, 0) = -2;
                    f3(-1, 1) = 4;                  f3(1,  1) = 4;
                                    f3(0,  2) = -3;

    Func d0("demosaic_0");
    Func d1("demosaic_1");
    Func d2("demosaic_2");
    Func d3("demosaic_3");

    Func output("demosaic_output");

    RDom r0(-2, 5, -2, 5);

    Func input_mirror = BoundaryConditions::mirror_image(input);

    d0(x, y) = cast<uint16_t>(clamp(sum(cast<int32_t>(input_mirror(x + r0.x, y + r0.y)) * f0(r0.x, r0.y)) / 8,  0, 65535));
    d1(x, y) = cast<uint16_t>(clamp(sum(cast<int32_t>(input_mirror(x + r0.x, y + r0.y)) * f1(r0.x, r0.y)) / 16, 0, 65535));
    d2(x, y) = cast<uint16_t>(clamp(sum(cast<int32_t>(input_mirror(x + r0.x, y + r0.y)) * f2(r0.x, r0.y)) / 16, 0, 65535));
    d3(x, y) = cast<uint16_t>(clamp(sum(cast<int32_t>(input_mirror(x + r0.x, y + r0.y)) * f3(r0.x, r0.y)) / 16, 0, 65535));

    Var c;

    output(x, y, c) = input(x, y);                                              // Initialize each channel to input mosaicked image

    RDom r1(0, input.width() / 2, 0, input.height() / 2); 
    
    // red
    output(r1.x * 2 + 1, r1.y * 2, 0) = d1(r1.x * 2 + 1, r1.y * 2);             // R at green in R row, B column
    output(r1.x * 2, r1.y * 2 + 1, 0) = d2(r1.x * 2, r1.y * 2 + 1);             // R at green in B row, R column
    output(r1.x * 2 + 1, r1.y * 2 + 1, 0) = d3(r1.x * 2 + 1, r1.y * 2 + 1);     // R at blue in B row, B column

    // green
    output(r1.x * 2, r1.y * 2, 1) = d0(r1.x * 2, r1.y * 2);                     // G at R locations
    output(r1.x * 2 + 1, r1.y * 2 + 1, 1) = d0(r1.x * 2 + 1, r1.y * 2 + 1);     // G at B locations

    // blue
    output(r1.x * 2, r1.y * 2 + 1, 2) = d1(r1.x * 2, r1.y * 2 + 1);             // B at green in B row, R column
    output(r1.x * 2 + 1, r1.y * 2, 2) = d2(r1.x * 2 + 1, r1.y * 2);             // B at green in R row, B column
    output(r1.x * 2, r1.y * 2, 2) = d3(r1.x * 2, r1.y * 2);                     // B at red in R row, R column

    Image<uint16_t> output_img(input.width(), input.height(), 3);

    output.realize(output_img);

    return output_img;
}


Image<uint8_t> finish(Image<uint16_t> input2) {

    // TODO: implement (or use implementations) of sum of the following steps
    // steps 4 and 7 are necessary

    // 1. Black-level subtraction
    // 2. Lens shading correction
    // 3. White balancing

    // 4. Demosaicking
    Image<uint16_t> input3 = Tools::load_image("../images/teapot.png");
    Image<uint16_t> input(input3.width() - 2, input3.height());
    for (int y = 0; y < input.height(); y++) {
        for (int x = 0; x < input.width(); x++) {
            input(x, y) = input3(x + 1, y, 0);
        }
    }
    std::cout << "HERE" << std::endl;
    Image<uint16_t> demosaic_output = demosaic(input);

    // 5. Chroma denoising
    // 6. Color correction
    
    // 7. Dynamic range compression
    // 8. Dehazing
    // 9. Global tone adjustment
    // 10. Chromatic aberration correction
    // 11. Sharpening
    // 12. Hue-specific color adjustments
    // 13. Dithering

    // The output image must have an RGB interleaved memory layout
    Func output;
    Var x, y, c;

    //int brighten_factor = 1;
    output(c, x, y) = cast<uint8_t>(clamp(cast<uint32_t>(demosaic_output(x, y, c)) / 256, 0, 255));

    Image<uint8_t> output_img(3, input.width(), input.height());

    output.realize(output_img);
    output_img.transpose(0, 1);
    output_img.transpose(1, 2);

    // for (int x = 0; x < output_img.width(); x++) {
    //     for (int y = 0; y < output_img.height(); y++) {
    //         std::cout << "val: " << (int)output_img(x, y, 0) << std::endl;
    //     }
    // }

    return output_img;
}