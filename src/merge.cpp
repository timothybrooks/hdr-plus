#ifndef HDRPLUS_MERGE_H_
#define HDRPLUS_MERGE_H_

#include "Halide.h"

using namespace Halide;

Image<uint16_t> merge(Image<uint16_t> imgs, Func alignment) {

    // TODO: figure out what we need to do lol
    // Weiner filtering or something like that...

    Func output;
    Var x, y;
    output(x, y) = imgs(x, y, 0);

    Image<uint16_t> output_img(imgs.extent(0), imgs.extent(1));

    output.realize(output_img);

    // for (int x = 0; x < imgs.extent(0); x++) {
    //     for (int y = 0; y < imgs.extent(1); y++) {
    //         std::cout << (int)output_img(x, y) << std::endl;
    //     }
    // }

    return output_img;
}

#endif