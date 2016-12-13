#include "util.h"

#include "Halide.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

Func box_down2(Func input) {

    Func output(input.name() + "_box_down2");
    
    Var x, y, n;
    RDom r(0, 2, 0, 2);

    // output with box filter and stride 2

    output(x, y, n) = u16(sum(u32(input(2*x + r.x, 2*y + r.y, n))) / 4);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(n).parallel(y).vectorize(x, 16);

    return output;
}

Func gauss_down4(Func input) {

    Func output(input.name() + "_gauss_down4");
    Func k("gauss_down4_filter");

    Var x, y, n;
    RDom r(-2, 5, -2, 5);

    // gaussian kernel

    k(x, y) = 0;

    k(-2,-2) = 2; k(-1,-2) =  4; k(0,-2) =  5; k(1,-2) =  4; k(2,-2) = 2;
    k(-2,-1) = 4; k(-1,-1) =  9; k(0,-1) = 12; k(1,-1) =  9; k(2,-1) = 4;
    k(-2, 0) = 5; k(-1, 0) = 12; k(0, 0) = 15; k(1, 0) = 12; k(2, 0) = 5;
    k(-2, 1) = 4; k(-1, 1) =  9; k(0, 1) = 12; k(1, 1) =  9; k(2, 1) = 4;
    k(-2, 2) = 2; k(-1, 2) =  4; k(0, 2) =  5; k(1, 2) =  4; k(2, 2) = 2;

    // output with applied kernel and stride 4

    output(x, y, n) = u16(sum(u32(input(4*x + r.x, 4*y + r.y, n) * k(r.x, r.y))) / 256);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    k.compute_root().parallel(y).parallel(x);

    output.compute_root().parallel(n).parallel(y).vectorize(x, 16);

    return output;
}

//takes in a 2D image
Func gauss_7x7(Func input, bool isFloat) {
    Func output(input.name() + "_gauss_7x7");
    Func k("gauss_7x7_filter");

    Var x, y;

    //gaussian kernel

    k(x,y) = f32(0.f);
    k(-3, -3) = 0.007507f; k(-2, -3) = 0.011815f; k(-1, -3) = 0.015509f; k(0, -3) = 0.016982f; k(1, -3) = 0.015509f; k(2, -3) = 0.011815f; k(3, -3) = 0.007507f;
    k(-3, -2) = 0.011815f; k(-2, -2) = 0.018594f; k(-1, -2) = 0.024408f; k(0, -2) = 0.026726f; k(1, -2) = 0.024408f; k(2, -2) = 0.018594f; k(3, -2) = 0.011815f;
    k(-3, -1) = 0.015509f; k(-2, -1) = 0.024408f; k(-1, -1) = 0.032041f; k(0, -1) = 0.035083f; k(1, -1) = 0.032041f; k(2, -1) = 0.024408f; k(3, -1) = 0.015509f;
    k(-3,  0) = 0.016982f; k(-2,  0) = 0.026726f; k(-1,  0) = 0.035083f; k(0,  0) = 0.038414f; k(1,  0) = 0.035083f; k(2,  0) = 0.026726f; k(3,  0) = 0.016982f;
    k(-3,  1) = 0.015509f; k(-2,  1) = 0.024408f; k(-1,  1) = 0.032041f; k(0,  1) = 0.035083f; k(1,  1) = 0.032041f; k(2,  1) = 0.024408f; k(3,  1) = 0.015509f;
    k(-3,  2) = 0.011815f; k(-2,  2) = 0.018594f; k(-1,  2) = 0.024408f; k(0,  2) = 0.026726f; k(1,  2) = 0.024408f; k(2,  2) = 0.018594f; k(3,  2) = 0.011815f;
    k(-3,  3) = 0.007507f; k(-2,  3) = 0.011815f; k(-1,  3) = 0.015509f; k(0,  3) = 0.016982f; k(1,  3) = 0.015509f; k(2,  3) = 0.011815f; k(3,  3) = 0.007507f;
    RDom r(-3, 7, -3, 7);
    if (isFloat) {
        output(x, y) = sum(input(x + r.x, y + r.y) * k(r.x, r.y));
    } else {
        output(x, y) = i32(sum(input(x + r.x, y + r.y) * k(r.x, r.y)));
    }

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    k.compute_root().parallel(y).parallel(x);
    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}