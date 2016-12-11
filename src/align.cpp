#include <string>
#include "align.h"
#include "Halide.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

/*
 * Returns an index to the nearest tile in the previous level of the pyramid
 * Would prefer (tile_e - 1) / DOWNSAMPLE_FACTOR, but CPP rounds division to 0
 */
inline Halide::Expr prev_tile(Halide::Expr t) { return (t - 1) / DOWNSAMPLE_FACTOR;}

/* 
 * Returns the image index given a tile and the inner index into the tile.
 */
inline Halide::Expr idx_layer(Halide::Expr t, Halide::Expr i) { return t * T_SIZE_2 / 2 + i; }

struct Point {
    Expr x, y;

    // Construct from a Tuple
    Point(Tuple t) : x(i16(t[0])), y(i16(t[1])) {}

    // Construct from a pair of Exprs
    Point(Expr x, Expr y) : x(i16(x)), y(i16(y)) {}

    // Construct from a call to a Func by treating it as a Tuple
    Point(FuncRef t) : Point(Tuple(t)) {}

    // Convert to a Tuple
    operator Tuple() const {
        return {x, y};
    }

    // Point addition
    Point operator+(const Point &other) const {
        return {x + other.x, y + other.y};
    }

    // Point subtraction
    Point operator-(const Point &other) const {
        return {x - other.x, y - other.y};
    }

    // Scalar multiplication
    Point operator*(const int n) const {
        return {n * x, n * y};
    }

};

typedef Point P;

Point operator*(const int n, const Point p) {
    return p * n;
}

Point operator-(const Point p) {
    return Point(-p.x, -p.y);
}

inline Point print(Point p) {
    return Point(print(p.x, p.y), p.y);
}

template <typename... Args>
inline Point print_when(Expr condition, Point p, Args&&... args) {
    return Point(print_when(condition, p.x, p.y, args...), p.y);
}

inline Point select(Expr condition, Point true_value, Point false_value) {
    return Point(select(condition, true_value.x, false_value.x), select(condition, true_value.y, false_value.y));
}

inline Point clamp(Point p, Point min_p, Point max_p) {
    return Point(clamp(p.x, min_p.x, max_p.x), clamp(p.y, min_p.y, max_p.y));
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

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

//try every offset in SEARCH_RANGE and record the score
//this version of the function assumes no previous offset.
//TODO: Avoid computation for layer 0
// Func L2_scores(Func pyramid_layer) {
//     assert(pyramid_layer.dimensions() == 3);
//     Func output(pyramid_layer.name() + "_L2_scores");
//     Var tile_x, tile_y, offset_x, offset_y, n;
//     RDom r(0, T_SIZE, 0, T_SIZE);
//     Expr component_dist = cast<int32_t>(pyramid_layer(tile_x * T_SIZE_2 + r.x, tile_y * T_SIZE_2 + r.y, 0)) - 
//                           cast<int32_t>(pyramid_layer(tile_x * T_SIZE_2 + r.x + offset_x, tile_y * T_SIZE_2 + r.y + offset_y, n));
//     output(tile_x, tile_y, offset_x, offset_y, n) = sum(component_dist * component_dist);

//     return output;
// }

//pyramid layer(x,y,n) -> uint16_t
//prev_best_offsets(is_y_offset,tileX,tileY,n) -> uint16_t
//this version of the function takes into account the previous offset
/*
Func L2_scores(Func pyramid_layer, Func prev_best_offsets) {
    assert(pyramid_layer.dimensions() == 3);
    assert(prev_best_offsets.dimensions() == 4);

    Func output(pyramid_layer.name() + "_L2_scores");
    Var tile_x, tile_y, offset_x, offset_y, n;
    
    Expr old_tile_x = tile_x / DOWNSAMPLE_FACTOR;
    Expr old_tile_y = tile_y / DOWNSAMPLE_FACTOR;
    Expr prev_offset_x = prev_best_offsets(0, old_tile_x, old_tile_y, n) * DOWNSAMPLE_FACTOR;
    Expr prev_offset_y = prev_best_offsets(1, old_tile_x, old_tile_y, n) * DOWNSAMPLE_FACTOR;

    RDom r(0, T_SIZE, 0, T_SIZE);
    //TODO avoid out of bounds here
    Expr component_dist = pyramid_layer((tile_x/2) * T_SIZE + r.x, (tile_y/2) * T_SIZE + r.y, 0) - pyramid_layer((tile_x/2) * T_SIZE + r.x - offset_x - prev_offset_x, (tile_y/2) * T_SIZE + r.y - offset_y - prev_offset_y , n + 1);
    output(tile_x, tile_y, offset_x, offset_y, n) = 2;//sum(component_dist * component_dist);
    return output;
}
*/

//offset_scores(tile_x, tile_y, offset_x, offset_y, n);
//best_offsets(is_y_offset, tile_x, tile_y, n)
// Func best_offsets(Func offset_scores) {
//     assert(offset_scores.dimensions() == 5);
//     Func output(offset_scores.name() + "_best_offsets");
//     Func best_scores(offset_scores.name() + "_best_scores");

//     Var is_y_offset, tile_x, tile_y, n;
//     output(is_y_offset, tile_x, tile_y, n) = cast<int16_t>(0);

//     RDom r0(-SEARCH_RANGE, 2 * SEARCH_RANGE, -SEARCH_RANGE, 2 * SEARCH_RANGE);
//     best_scores(tile_x, tile_y, n) = minimum(offset_scores(tile_x, tile_y, r0.x, r0.y, n));

//     RDom r1(-SEARCH_RANGE, 2 * SEARCH_RANGE, -SEARCH_RANGE, 2 * SEARCH_RANGE);

//     output(0, tile_x, tile_y, n) = 
//         select(offset_scores(tile_x, tile_y, r1.x, r1.y, n) == best_scores(tile_x, tile_y, n), cast<int16_t>(r1.x), output(0, tile_x, tile_y, n));
//     output(1, tile_x, tile_y, n) = 
//         select(offset_scores(tile_x, tile_y, r1.x, r1.y, n) == best_scores(tile_x, tile_y, n), cast<int16_t>(r1.y), output(1, tile_x, tile_y, n));
//     return output;
// }

/*
Func best_offsets(Func offset_scores, Func prev_best_offsets) {
    assert(offset_scores.dimensions() == 5);
    Func output(offset_scores.name() + "_best_offsets");
    Func best_scores(offset_scores.name() + "_best_scores");
    
    Var is_y_offset, tile_x, tile_y, n;
    output(is_y_offset, tile_x, tile_y, n) = (uint16_t) 0;

    //r0: offset_x, offset_y
    RDom r0(-SEARCH_RANGE, 2 * SEARCH_RANGE + 1, -SEARCH_RANGE, 2 * SEARCH_RANGE + 1);
    best_scores(tile_x, tile_y, n) = minimum(offset_scores(tile_x, tile_y, r0.x, r0.y, n));

    Expr old_tile_x = tile_x/DOWNSAMPLE_FACTOR;
    Expr old_tile_y = tile_y/DOWNSAMPLE_FACTOR;
    Expr prev_offset_x = prev_best_offsets(0, old_tile_x, old_tile_y, n) * DOWNSAMPLE_FACTOR;
    Expr prev_offset_y = prev_best_offsets(1, old_tile_x, old_tile_y, n) * DOWNSAMPLE_FACTOR;

    //r1: offset_x, offset_y
    RDom r1(-SEARCH_RANGE, 2 * SEARCH_RANGE + 1, -SEARCH_RANGE, 2 * SEARCH_RANGE + 1);
    output(0, tile_x, tile_y, n) = 
        select(offset_scores(tile_x, tile_y, r1.x, r1.y, n) == best_scores(tile_x, tile_y, n), r1.x + prev_offset_x, output(0, tile_x, tile_y, n));
    output(1, tile_x, tile_y, n) = 
        select(offset_scores(tile_x, tile_y, r1.x, r1.y, n) == best_scores(tile_x, tile_y, n), r1.y + prev_offset_y, output(1, tile_x, tile_y, n));
    return output;
}
*/

    // layer_0.vectorize(x, 8);
    // layer_0.parallel(y);
    // layer_0.compute_root();

    
    // Func layer_1 = gauss_down4(layer_0);
    // Func layer_2 = gauss_down4(layer_1);

    // //offset_scores_n(tile_x, tile_y, marginal_offset_x, marginal_offset_y, n) marginal offsets are relative to inherited offset
    
    // Func offset_scores_2 = L2_scores(layer_2);
    // Func best_offsets_2 = best_offsets(offset_scores_2);

    // Func offset_scores_1 = L2_scores(layer_1, best_offsets_2);
    // Func best_offsets_1 = best_offsets(offset_scores_1, best_offsets_2);

    // Func offset_scores_0 = L2_scores(layer_0, best_offsets_1);
    // Func best_offsets_0 = best_offsets(offset_scores_0, best_offsets_1);
    
    // Func offset_scores_0("offset_scores_0");
    // offset_scores_0(tile_x, tile_y, offset_x, offset_y, n) = L2_scores(layer_0)(tile_x, tile_y, offset_x, offset_y, n);

    // //offset_scores_0 schedule
    // offset_scores_0.vectorize(tile_x, 8);
    // offset_scores_0.fuse(offset_x, offset_y, offset);
    // offset_scores_0.parallel(offset);
    // offset_scores_0.compute_root();

    //best_offsets(offset_scores_0)(is_y_offset, tile_x, tile_y, n);
    //alignment(is_y_offset, tile_x, tile_y, n) = cast<uint16_t>(4);

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
          .parallel(n)
          .parallel(ty)
          .vectorize(tx, 16);

    min_scores.compute_root()
              .parallel(n)
              .parallel(ty)
              .vectorize(tx, 16);

    alignment.compute_root()
             .parallel(n)
             .parallel(ty)
             .vectorize(tx, 16);

    return alignment;
}

Func align(Image<uint16_t> imgs) {

    int num_tx = imgs.width() / T_SIZE_2 - 1;
    int num_ty = imgs.height() / T_SIZE_2 - 1;
    int num_alts = imgs.extent(2) - 1;

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
    alignment(tx, ty, n) = 2 * P(alignment_0(clamp(tx, 0, num_tx), clamp(ty, 0, num_ty), n));

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    alignment_3.compute_root()
               .parallel(n)
               .parallel(ty)
               .vectorize(tx, 16);

    alignment.compute_root()
             .parallel(n)
             .parallel(ty)
             .vectorize(tx, 16);

    // realize image

    // Image<int16_t> alignment_img_x(num_tx + 2, num_ty + 2, num_alts);
    // Image<int16_t> alignment_img_y(num_tx + 2, num_ty + 2, num_alts);

    // alignment_img_x.set_min(-1, -1, 1);
    // alignment_img_y.set_min(-1, -1, 1);

    // Realization r = {alignment_img_x, alignment_img_y};

    // alignment.realize(r);

    // TODO: Get rid of this when you can

    Func temp("temp");
    Var a, b, c, d;
    temp(a, b, c, d) = select(a == 1, P(alignment(b, c, d)).y, P(alignment(b, c, d)).x);
    temp.vectorize(b,8);
    temp.parallel(c);
    temp.compute_root();
    
    return temp;
}