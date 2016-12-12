#ifndef HDRPLUS_ALIGN_H_
#define HDRPLUS_ALIGN_H_

#define T_SIZE 32
#define T_SIZE_2 16

#include "Halide.h"

/* 
 * Returns the upper (for y input) or left (for x input) tile that an image
 * index touches.
 */
inline Halide::Expr tile_0(Halide::Expr e) { return e / T_SIZE_2 - 1; }

/* 
 * Returns the lower (for y input) or right (for x input) tile that an image
 * index touches.
 */
inline Halide::Expr tile_1(Halide::Expr e) { return e / T_SIZE_2; }

/* 
 * Returns the inner index into the upper (for y input) or left (for x input)
 * tile that an image index touches.
 */
inline Halide::Expr idx_0(Halide::Expr e) { return e % T_SIZE_2  + T_SIZE_2; }

/* 
 * Returns the inner index into the lower (for y input) or right (for x input)
 * tile that an image index touches.
 */
inline Halide::Expr idx_1(Halide::Expr e) { return e % T_SIZE_2; }

/* 
 * Returns the image index given a tile and the inner index into the tile.
 */
inline Halide::Expr idx_im(Halide::Expr t, Halide::Expr i) { return t * T_SIZE_2 + i; }

/* 
 * Returns the image index given a tile and the inner index into the tile.
 */
inline Halide::Expr idx_layer(Halide::Expr t, Halide::Expr i) { return t * T_SIZE_2 / 2 + i; }

// I wonder if it would be appropriate to make these images 'const' because they should not be modified...
// same idea applies to the merge and finish components

Halide::Func box_down2(Halide::Func input);

Halide::Func align(Halide::Image<uint16_t> imgs);

#endif