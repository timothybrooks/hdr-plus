#include "merge.h"

#include "Halide.h"

using namespace Halide;
//alignment(isYOffset, tile_x, tile_y, n)
Image<uint16_t> merge(Image<uint16_t> imgs, Func alignment) {

    // TODO: figure out what we need to do lol
    // Weiner filtering or something like that...

    Func clamped_imgs("clamped_imgs");
    clamped_imgs = BoundaryConditions::mirror_image(imgs);
    Var tile_x, tile_y, ix, iy, x, y;

    Expr x_im = tile_x * TILE_SIZE/2 + ix;
    Expr y_im = tile_y * TILE_SIZE/2 + iy;
    Func merge_temporal("merge_temporal");
    merge_temporal(ix, iy, tile_x, tile_y) = clamped_imgs(x_im, y_im, 0);
    RDom rn(1,imgs.extent(2)-1);
    merge_temporal(ix,iy,tile_x,tile_y) += clamped_imgs(x_im + alignment(0,tile_x,tile_y,rn),
                                                        y_im + alignment(1,tile_x,tile_y,rn),                                                 rn);
    merge_temporal(ix,iy,tile_x,tile_y) /= cast<uint16_t>(imgs.extent(2));

    Func merge_spatial("merge_spatial");
    merge_spatial(x,y) = merge_temporal(x % TILE_SIZE, y % TILE_SIZE, x / TILE_SIZE * 2, y / TILE_SIZE * 2);

    Image<uint16_t> merged_image(imgs.extent(0), imgs.extent(1));
    merge_spatial.realize(merged_image);
    
    return merged_image;
}