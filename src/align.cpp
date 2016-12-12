#include <string>
#include "align.h"
#include "Halide.h"
#include "Point.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

/*
 * Returns an index to the nearest tile in the previous level of the pyramid.
 */
inline Halide::Expr prev_tile(Halide::Expr t) { return (t - 1) / 4; }

Func box_down2(Func input) {

    assert(input.dimensions() == 3);

    Func output(input.name() + "_box_down2");
    Var x, y, n;

    RDom r(0, 2, 0, 2);

    output(x, y, n) = u16(sum(u32(input(2*x + r.x, 2*y + r.y, n))) / 4);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    output.compute_root()
          .parallel(n)
          .parallel(y)
          .vectorize(x, 16);

    return output;
}

Func gauss_down4(Func input) {

    assert(input.dimensions() == 3);

    Func output(input.name() + "_gauss_down4");
    Var x, y, n;

    Func k("gauss_down4_filter");
    
    k(x, y) = 0;

    k(-2,-2) = 2; k(-1,-2) =  4; k(0,-2) =  5; k(1,-2) =  4; k(2,-2) = 2;
    k(-2,-1) = 4; k(-1,-1) =  9; k(0,-1) = 12; k(1,-1) =  9; k(2,-1) = 4;
    k(-2, 0) = 5; k(-1, 0) = 12; k(0, 0) = 15; k(1, 0) = 12; k(2, 0) = 5;
    k(-2, 1) = 4; k(-1, 1) =  9; k(0, 1) = 12; k(1, 1) =  9; k(2, 1) = 4;
    k(-2, 2) = 2; k(-1, 2) =  4; k(0, 2) =  5; k(1, 2) =  4; k(2, 2) = 2;

    RDom r(-2, 5, -2, 5);

    output(x, y, n) = u16(sum(u32(input(4*x + r.x, 4*y + r.y, n) * k(r.x, r.y))) / 256);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    k.compute_root()
     .parallel(y)
     .parallel(x);

    output.compute_root()
          .parallel(n)
          .parallel(y)
          .vectorize(x, 16);

    return output;
}

Func align_layer(Func layer, Func prev_alignment, Point prev_min, Point prev_max) {

    Var xi, yi, tx, ty, n;

    RDom r0(0, 16, 0, 16);

    Point prev_offset = 4 * clamp(P(prev_alignment(prev_tile(tx), prev_tile(ty), n)), prev_min, prev_max);

    Expr x0 = idx_layer(tx, r0.x);
    Expr y0 = idx_layer(ty, r0.y);

    Expr x = x0 + prev_offset.x + xi;
    Expr y = y0 + prev_offset.y + yi;

    Expr ref_val = layer(x0, y0, 0);
    Expr alt_val = layer(x, y, n);

    Expr dist = i64(ref_val) - i64(alt_val);

    Func scores(layer.name() + "_align_scores");

    scores(xi, yi, tx, ty, n) = sum(dist * dist);

    Func min_scores(layer.name() + "_min_align_scores");

    RDom r1(-4, 9, -4, 9);

    min_scores(tx, ty, n) = minimum(scores(r1.x, r1.y, tx, ty, n));

    Func alignment(layer.name() + "_alignment");

    alignment(tx, ty, n) = P(0, 0);

    alignment(tx, ty, n) = select(scores(r1.x, r1.y, tx, ty, n) == min_scores(tx, ty, n),
                                  P(r1.x, r1.y) + prev_offset,
                                  alignment(tx, ty, n));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    scores.compute_root()
          .parallel(ty)
          .vectorize(tx, 16);

    min_scores.compute_root()
              .parallel(ty)
              .vectorize(tx, 16);

    alignment.compute_root()
             .parallel(ty)
             .vectorize(tx, 16);

    return alignment;
}

Func align(Image<uint16_t> imgs) {

    Func imgs_mirror = BoundaryConditions::mirror_interior(imgs);

    Func layer_0 = box_down2(imgs_mirror);
    Func layer_1 = gauss_down4(layer_0);
    Func layer_2 = gauss_down4(layer_1);

    Point search = P(4, 4);

    Point min_2 = -search;
    Point min_1 = 4 * min_2 - search;
    Point min_0 = 4 * min_1 - search;

    Point max_2 = search;
    Point max_1 = 4 * max_2 + search;
    Point max_0 = 4 * max_1 + search;

    Func alignment_3("layer_3_alignment");
    Var tx, ty, n;

    alignment_3(tx, ty, n) = P(0, 0);

    Func alignment_2 = align_layer(layer_2, alignment_3, min_2, max_2);
    Func alignment_1 = align_layer(layer_1, alignment_2, min_1, max_1);
    Func alignment_0 = align_layer(layer_0, alignment_1, min_0, max_0);

    Func alignment("alignment");

    int num_tx = imgs.width() / T_SIZE_2 - 1;
    int num_ty = imgs.height() / T_SIZE_2 - 1;

    alignment(tx, ty, n) = 2 * P(alignment_0(clamp(tx, 0, num_tx), clamp(ty, 0, num_ty), n));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    alignment_3.compute_root()
               .parallel(ty)
               .vectorize(tx, 16);

    alignment.compute_root()
             .parallel(ty)
             .vectorize(tx, 16);
    
    return alignment;
}