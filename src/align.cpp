#include <string>
#include "align.h"
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
    output(x, y, n) = sum(input(2*x + r.x, 2*y + r.y, n)) / cast<uint16_t>(4);

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
    k(x,y) = cast<uint16_t>(0);
    k(0, 0) = cast<uint16_t>(4);
    k(-1, -1) = cast<uint16_t>(1); k(-1, 1) = cast<uint16_t>(1); k(1, -1) = cast<uint16_t>(1); k(1, 1) = cast<uint16_t>(1);
    k(-1,  0) = cast<uint16_t>(1); k( 0, 1) = cast<uint16_t>(1); k(0, -1) = cast<uint16_t>(1); k(1, 0) = cast<uint16_t>(2);

    RDom r(-1, 3, -1, 3);
    output(x, y, n) = sum(input(2*x + r.x, 2*y + r.y, n) * k(r.x, r.y)) / cast<uint16_t>(16);
    
    // TODO: schedule (unless we are going to fuse scheduling...)
    return output;
}

Func gauss_down4(Func input) {
    assert(input.dimensions() == 3);

    Func output(input.name() + "_gauss_down4");
    Var x, y, n;

    // TODO: compute in two passes
    // we can optimize this later
    Func k("gauss_down4_filter");
    k(x,y) = cast<uint16_t>(0);
    k(-2,-2) = cast<uint16_t>(1); k(2,-2) = cast<uint16_t>(1); k(-2, 2) = cast<uint16_t>(1); k(2, 2) = cast<uint16_t>(1);
    k(-1,-2) = cast<uint16_t>(4); k(1,-2) = cast<uint16_t>(4); k(-2,-1) = cast<uint16_t>(4); k(2,-1) = cast<uint16_t>(4); k(-2, 1) = cast<uint16_t>(4); k(2, 1) = cast<uint16_t>(4); k(-1, 2) = cast<uint16_t>(4); k(1, 2) = cast<uint16_t>(4);
    k(0,-2) = cast<uint16_t>(6); k(-2, 0) = cast<uint16_t>(6); k(2, 0) = cast<uint16_t>(6); k(0, 2) = cast<uint16_t>(6);
    k(-1,-1) = cast<uint16_t>(16); k(1,-1) = cast<uint16_t>(16); k(-1, 1) = cast<uint16_t>(16); k(1, 1) = cast<uint16_t>(16);
    k(0,-1) = cast<uint16_t>(24); k(-1, 0) = cast<uint16_t>(24); k(1, 0) = cast<uint16_t>(24); k(0, 1) = cast<uint16_t>(24);
    k(0, 0) = cast<uint16_t>(36);
    RDom r(-2, 5, -2, 5);
    k.vectorize(x,16);
    k.parallel(y);
    k.compute_root();
    output(x, y, n) = sum(input(4*x + r.x, 4*y + r.y, n) * k(r.x, r.y)) / cast<uint16_t>(256);
    return output;
}

//try every offset in SEARCH_RANGE and record the score
//this version of the function assumes no previous offset.
//TODO: Avoid computation for layer 0
Func L2_scores(Func pyramid_layer) {
    assert(pyramid_layer.dimensions() == 3);
    Func output(pyramid_layer.name() + "_L2_scores");
    Var tile_x, tile_y, offset_x, offset_y, n;
    RDom r(0, T_SIZE, 0, T_SIZE);
    Expr component_dist = cast<int32_t>(pyramid_layer(tile_x * T_SIZE_2 + r.x, tile_y * T_SIZE_2 + r.y, 0)) - 
                          cast<int32_t>(pyramid_layer(tile_x * T_SIZE_2 + r.x + offset_x, tile_y * T_SIZE_2 + r.y + offset_y, n));
    output(tile_x, tile_y, offset_x, offset_y, n) = sum(component_dist * component_dist);

    return output;
}

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
Func best_offsets(Func offset_scores) {
    assert(offset_scores.dimensions() == 5);
    Func output(offset_scores.name() + "_best_offsets");
    Func best_scores(offset_scores.name() + "_best_scores");

    Var is_y_offset, tile_x, tile_y, n;
    output(is_y_offset, tile_x, tile_y, n) = cast<int16_t>(0);

    RDom r0(-SEARCH_RANGE, 2 * SEARCH_RANGE, -SEARCH_RANGE, 2 * SEARCH_RANGE);
    best_scores(tile_x, tile_y, n) = minimum(offset_scores(tile_x, tile_y, r0.x, r0.y, n));

    RDom r1(-SEARCH_RANGE, 2 * SEARCH_RANGE, -SEARCH_RANGE, 2 * SEARCH_RANGE);

    output(0, tile_x, tile_y, n) = 
        select(offset_scores(tile_x, tile_y, r1.x, r1.y, n) == best_scores(tile_x, tile_y, n), cast<int16_t>(r1.x), output(0, tile_x, tile_y, n));
    output(1, tile_x, tile_y, n) = 
        select(offset_scores(tile_x, tile_y, r1.x, r1.y, n) == best_scores(tile_x, tile_y, n), cast<int16_t>(r1.y), output(1, tile_x, tile_y, n));
    return output;
}

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

Func align(Image<uint16_t> imgs) {
    Var x, y, n, is_y_offset, tile, tile_x, tile_y, offset, offset_x, offset_y;
    // Func clamped_imgs("clamped_imgs");
    // clamped_imgs = BoundaryConditions::mirror_image(imgs);

    // //TODO pad layers to prevent out of bounds for alternates when calculating scores.
    // Func layer_0("layer_0");
    // layer_0(x, y, n) = gauss_down4(clamped_imgs)(x, y, n);
    
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
    
    Func alignment("alignment");
    alignment(is_y_offset, tile_x, tile_y, n) = 0;//best_offsets(offset_scores_0)(is_y_offset, tile_x, tile_y, n);
    //alignment(is_y_offset, tile_x, tile_y, n) = cast<uint16_t>(4);

    
    //alignment schedule
    alignment.vectorize(tile_x,8);
    alignment.parallel(tile_y);
    alignment.compute_root();
    
    return alignment;
}