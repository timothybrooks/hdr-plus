#include "Halide.h"

using namespace Halide;

Image<uint8_t> finish(Image<uint16_t> input) {

    // TODO: implement (or use implementations) of sum of the following steps
    // steps 4 and 7 are necessary

    // 1. Black-level subtraction
    // 2. Lens shading correction
    // 3. White balancing
    // 4. Demosaicking
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

    int brighten_factor = 14;
    output(c, x, y) = cast<uint8_t>(clamp(brighten_factor * cast<uint32_t>(input(x, y)) / 256, 0, 255));

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