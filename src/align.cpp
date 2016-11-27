#include <string>

#include "Halide.h"

using namespace Halide;

// This file includes the implementation of the function align()
// The function is declared at the bottom. Many helper functions are declared above it.
// align() is the only function provided by the header file.

Func box_down2(Func input) {
    assert(input.dimensions() == 3);

    Func output(input.name() + "_box_down2");
    Var x, y, n;

    RDom r(0, 2, 0, 2);
    output(x, y, n) = sum(input(2*x + r.x, 2*y + r.y, n)) / 4;

    // TODO: schedule (unless we are going to fuse scheduling...)
    return output;
}

Func gauss_down2(Func input) {
    assert(input.dimensions() == 3);

    Func output(input.name() + "_gauss_down2");
    Var x, y, n;

    // TODO: compute in two passes
    // we can optimize this later
    Func k;
    k(x,y) = 0;
    k(0, 0) = 4;
    k(-1, -1) = 1; k(-1, 1) = 1; k(1, -1) = 1; k(1, 1) = 1;
    k(-1,  0) = 1; k( 0, 1) = 1; k(0, -1) = 1; k(1, 0) = 2;

    RDom r(-1, 3, -1, 3);
    output(x, y, n) = sum(input(2*x + r.x, 2*y + r.y, n) * k(r.x, r.y)) / 16;
    
    // TODO: schedule (unless we are going to fuse scheduling...)
    return output;
}

Func gauss_down4(Func input) {
    assert(input.dimensions() == 3);

    Func output(input.name() + "_gauss_down4");
    Var x, y, n;

    // TODO: compute in two passes
    // we can optimize this later
    Func k;
    k(x,y) = 0;
    k(-2,-2) = 1; k(2,-2) = 1; k(-2, 2) = 1; k(2, 2) = 1;
    k(-1,-2) = 4; k(1,-2) = 4; k(-2,-1) = 4; k(2,-1) = 4; k(-2, 1) = 4; k(2, 1) = 4; k(-1, 2) = 4; k(1, 2) = 4;
    k(0,-2) = 6; k(-2, 0) = 6; k(2, 0) = 6; k(0, 2) = 6;
    k(-1,-1) = 16; k(1,-1) = 16; k(-1, 1) = 16; k(1, 1) = 16;
    k(0,-1) = 24; k(-1, 0) = 24; k(1, 0) = 24; k(0, 1) = 24;
    k(0, 0) = 36;

    RDom r(-2, 5, -2, 5);
    output(x, y, n) = sum(input(4*x + r.x, 4*y + r.y, n) * k(r.x, r.y)) / 256;
    
    // TODO: schedule (unless we are going to fuse scheduling...)
    return output;
}

Func align(Image<uint16_t> imgs) {
    Func layer_0 = box_down2(Func(imgs));
    Func layer_1 = gauss_down2(layer_0);
    Func layer_2 = gauss_down4(layer_1);
    Func layer_3 = gauss_down4(layer_2);

    // TODO: implement a lot of stuff for alignment...

    Func alignment("alignment");    // will have the dimmensionality: coordinate (0 or 1) * tile_x * tile_y * alternate image index
    return alignment;
}