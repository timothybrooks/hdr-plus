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

    Func output;
    Var x, y, c;
    output(x, y, c) = cast<uint8_t>(input(x, y));

    Image<uint8_t> output_img(input.extent(0), input.extent(1), 3);

    output.realize(output_img);

    // for (int y = 0; y < input.extent(0); y++) {
    //     for (int x = 0; x < input.extent(1); x++) {
    //         std::cout << (int)output_img(x, y) << std::endl;
    //     }
    // }

    return output_img;
}