#include "align.h"

#include <string>
#include "Halide.h"
#include "Point.h"
#include "util.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

/*
 * Returns an index to the nearest tile in the previous level of the pyramid.
 */
inline Halide::Expr prev_tile(Halide::Expr t) { return (t - 1) / 4; }

/*
 *
 */
Func align_layer(Func layer, Func prev_alignment, Point prev_min, Point prev_max) {

    Func scores(layer.name() + "_scores");
    Func alignment(layer.name() + "_alignment");

    Var xi, yi, tx, ty, n;
    RDom r0(0, 16, 0, 16);              // reduction over pixels in tile
    RDom r1(-4, 8, -4, 8);              // reduction over search region; extent clipped to 8 for SIMD vectorization

    // offset from the alignment of the previous layer, scaled to this layer

    Point prev_offset = 4 * clamp(P(prev_alignment(prev_tile(tx), prev_tile(ty), n)), prev_min, prev_max);

    // indices into layer at a specific tile indices and offsets

    Expr x0 = idx_layer(tx, r0.x);
    Expr y0 = idx_layer(ty, r0.y);

    Expr x = x0 + prev_offset.x + xi;
    Expr y = y0 + prev_offset.y + yi;

    // values and L1 distance between reference and alternate layers at specific pixel

    Expr ref_val = layer(x0, y0, 0);
    Expr alt_val = layer(x, y, n);

    Expr dist = abs(i32(ref_val) - i32(alt_val));

    // sum of L1 distances over each pixel in a tile, for the offset specified by xi, yi

    scores(xi, yi, tx, ty, n) = sum(dist);

    // alignment offset for each tile (offset where score is minimum)

    alignment(tx, ty, n) = P(argmin(scores(r1.x, r1.y, tx, ty, n))) + prev_offset;

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    scores.compute_root().parallel(ty).vectorize(xi, 8);

    alignment.compute_root().parallel(ty).vectorize(tx, 16);

    return alignment;
}

/*
 *
 */
Func align(const Image<uint16_t> imgs) {

    Func alignment_3("layer_3_alignment");
    Func alignment("alignment");

    Var tx, ty, n;

    // mirror input image with overlapping edges

    Func imgs_mirror = BoundaryConditions::mirror_interior(imgs);

    // downsampled layers for alignment

    Func layer_0 = box_down2(imgs_mirror, "layer_0");
    Func layer_1 = gauss_down4(layer_0, "layer_1");
    Func layer_2 = gauss_down4(layer_1, "layer_2");

    // min and max search regions

    Point search = P(4, 4);

    Point min_2 = -search;
    Point min_1 = 4 * min_2 - search;
    Point min_0 = 4 * min_1 - search;

    Point max_2 = search;
    Point max_1 = 4 * max_2 + search;
    Point max_0 = 4 * max_1 + search;

    // initial alignment of previous layer is 0, 0

    alignment_3(tx, ty, n) = P(0, 0);

    // hierarchal alignment functions

    Func alignment_2 = align_layer(layer_2, alignment_3, min_2, max_2);
    Func alignment_1 = align_layer(layer_1, alignment_2, min_1, max_1);
    Func alignment_0 = align_layer(layer_0, alignment_1, min_0, max_0);

    // number of tiles in the x and y dimensions

    int num_tx = imgs.width() / T_SIZE_2 - 1;
    int num_ty = imgs.height() / T_SIZE_2 - 1;

    // final alignment offsets for the original mosaic image

    alignment(tx, ty, n) = 2 * P(alignment_0(clamp(tx, 0, num_tx), clamp(ty, 0, num_ty), n));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    alignment_3.compute_root().parallel(ty).vectorize(tx, 16);

    alignment.compute_root().parallel(ty).vectorize(tx, 16);
    
    return alignment;
}