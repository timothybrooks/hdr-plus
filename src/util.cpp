#include "util.h"

#include "Halide.h"

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

//takes in a 2D image
Func gauss_7x7(Func input, std::string name) {

    Func output(name);
    Func k(name + "_filter");

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

    Expr val = sum(input(x + r.x, y + r.y) * k(r.x, r.y));

    if (input.output_types()[0] == UInt(16)) val = u16(val);

    output(x, y) = val;

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    k.compute_root().parallel(y).parallel(x);

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

/*
 * Computes difference between two functions
 */
Func diff(Func im1, Func im2, std::string name) {

    Func output(name);

    Var x, y;

    output(x,y) = i32(im1(x,y)) - i32(im2(x,y));

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

    if (input.dimensions() == 2) {
        output(x, y) = u16(select(input(x, y) < cutoff,
                            gamma_toe * input(x, y),
                            gamma_fac * pow(input(x, y), gamma_pow) + gamma_con));
    }
    else {
        output(x, y, c) = u16(select(input(x, y, c) < cutoff,
                            gamma_toe * input(x, y, c),
                            gamma_fac * pow(input(x, y, c), gamma_pow) + gamma_con));
    }

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

    return output;
}

Func gamma_inverse(Func input) {

    Func output("gamma_inverse_output");

    Var x, y, c;

    // constants for inverse gamma correction

    int cutoff = 2575;                   // ceil(1/0.00304 * UINT16_MAX)
    float gamma_toe = 0.0774;            // 1 / 12.92
    float gamma_pow = 2.4;
    float gamma_fac = 57632.49226;       // 1 / 1.055 ^ gamma_pow * U_INT16_MAX;
    float gamma_con = 0.055;

    if (input.dimensions() == 2) {
        output(x, y) = u16(select(input(x, y) < cutoff,
                            gamma_toe * input(x, y),
                            pow(f32(input(x, y)) / 65535.f + gamma_con, gamma_pow) * gamma_fac));
    }
    else {
        output(x, y, c) = u16(select(input(x, y, c) < cutoff,
                            gamma_toe * input(x, y),
                            pow(f32(input(x, y, c)) / 65535.f + gamma_con, gamma_pow) * gamma_fac));
    }

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root().parallel(y).vectorize(x, 16);

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

Func median_filter_3x3(Func input) {
    Func output(input.name() + "median_filtered");
    Var x, y, c;
    output(x, y, c) = input(x, y, c);
    
    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    //TODO

    return output;
}