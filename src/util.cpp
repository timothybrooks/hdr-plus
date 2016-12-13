#include "util.h"

#include "Halide.h"
#include <vector>
#include <algorithm>

using namespace Halide;
using namespace Halide::ConciseCasts;

Func box_down2(Func input, std::string name) {

    Func output(name);
    
    Var x, y, n;
    RDom r(0, 2, 0, 2);

    // output with box filter and stride 2

    output(x, y, n) = u16(sum(u32(input(2*x + r.x, 2*y + r.y, n))) / 4);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

Func gauss_down4(Func input, std::string name) {

    Func output(name);
    Func k(name + "_filter");

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

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

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

Func diff(Func im1, Func im2) {
    Func output(im1.name() + "_laplace");
    Var x, y;
    output(x,y) = i32(im1(x,y)) - i32(im2(x,y));
    return output;
}

Func rgb_to_yuv(Func input) {
    Func output("yuv_from_rgb_output");
    Var x, y, c;
    Expr r = input(x, y, 0);
    Expr g = input(x, y, 1);
    Expr b = input(x, y, 2);
    output(x, y, c) = f32(0);
    output(x, y, 0) =   .299f    * r + .587f    * g + .114f    * b; //Y
    output(x, y, 1) = - .168935f * r - .331655f * g + .500590f * b; //U
    output(x, y, 2) =   .499813f * r - .418531f * g - .081282f * b; //V
    
    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    //TODO

    return output;
}

Func yuv_to_rgb(Func input) {
    Func output("rgb_from_yuv_output");
    Var x, y, c;
    Expr Y = input(x, y, 0);
    Expr U = input(x, y, 1);
    Expr V = input(x, y, 2);
    output(x, y, c) = u16(0);
    output(x, y, 0) = u16_sat(Y + 1.403f * V            ); //r
    output(x, y, 1) = u16_sat(Y -  .344f * U - .714f * V); //g
    output(x, y, 2) = u16_sat(Y + 1.770f * U            ); //b

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    //TODO

    return output;
}

Image<float> bilateral_filter(Image<float> input, int width, int height) {
    
    Func k("gauss_kernel");
    Func weights("bilateral_weights");
    Func total_weights("bilateral_total_weights");
    Func bilateral("bilateral");
    Func output("bilateral_filter_output");

    Var x, y, dx, dy, c;

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

    Expr dist = f32(abs(input(x, y, c) - input_mirror(x + dx, y + dy, c))) / 65535.f;

    Expr score = 1.f - dist * dist;

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

    total_weights.compute_root().parallel(y).vectorize(x, 16);

    bilateral.compute_root().parallel(y).vectorize(x, 16);

    output.compute_root().parallel(y).vectorize(x, 16);

    output.update(0).parallel(y).vectorize(x, 16);
    output.update(1).parallel(y).vectorize(x, 16);

    Image<float> out_img = output.realize(width, height, 3);

    return out_img;
}