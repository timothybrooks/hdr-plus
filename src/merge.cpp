#include "merge.h"
#include "align.h"

#include "Halide.h"

// #include "../include/stb_image_write.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

/*
 * TODO: add function description here and in header
 * alignment(isYOffset, tx, ty, n)
 */
Image<uint16_t> merge(Image<uint16_t> imgs, Func alignment) {

    // TODO: Weiner filtering or something like that...

    int w = imgs.width();
    int h = imgs.height();
    int num_imgs = imgs.extent(2);
    int num_alts = num_imgs - 1;

    // aligned x and y

    Var ix, iy, tx, ty;

    RDom r(1, num_alts);

    Expr al_x = idx_im(tx, ix) + alignment(0, tx, ty, r);
    Expr al_y = idx_im(ty, iy) + alignment(1, tx, ty, r);

    Func imgs_bound = BoundaryConditions::constant_exterior(imgs, 0);

    // temporal merge function using averaging

    Func merge_temporal("merge_temporal");

    // initialize to reference image
    merge_temporal(ix, iy, tx, ty) = imgs_bound(idx_im(tx, ix), idx_im(ty, iy), 0);         

    // add alternate images
    merge_temporal(ix, iy, tx, ty) = u16((u32(merge_temporal(ix, iy, tx, ty))
                                    + sum(u32(imgs_bound(al_x, al_y, r)))) / num_imgs);

    ///////////////////////////////////////////////////////////////////////////
    // spatial merging
    ///////////////////////////////////////////////////////////////////////////

    // coefficient (raised cosine window)

    Func coef("coef_raised_cosine");
    Var n;

    float pi = 3.141592f;
    coef(n) = 0.5f - 0.5f * cos(2 * pi * (n + 0.5f) / T_SIZE);

    // tile coefficients

    Var x, y;

    Expr coef_00 = coef(idx_0(x)) * coef(idx_0(y));
    Expr coef_10 = coef(idx_1(x)) * coef(idx_0(y));
    Expr coef_01 = coef(idx_0(x)) * coef(idx_1(y));
    Expr coef_11 = coef(idx_1(x)) * coef(idx_1(y));

    // temporal merge values

    Expr val_00 = merge_temporal(idx_0(x), idx_0(y), tile_0(x), tile_0(y));
    Expr val_10 = merge_temporal(idx_1(x), idx_0(y), tile_1(x), tile_0(y));
    Expr val_01 = merge_temporal(idx_0(x), idx_1(y), tile_0(x), tile_1(y));
    Expr val_11 = merge_temporal(idx_1(x), idx_1(y), tile_1(x), tile_1(y));

    // spatial merge function using raised cosine window

    Func merge_spatial("merge_spatial");

    merge_spatial(x, y) = u16(coef_00 * val_00
                            + coef_10 * val_10
                            + coef_01 * val_01
                            + coef_11 * val_11);

    ///////////////////////////////////////////////////////////////////////////
    // schedule
    ///////////////////////////////////////////////////////////////////////////

    merge_temporal.compute_root()
                  .parallel(ty)
                  .parallel(tx)
                  .vectorize(ix, 16);

    coef.compute_root()
        .vectorize(n, 16);

    merge_spatial.compute_root()
                 .parallel(y)
                 .vectorize(x, 16);

    ///////////////////////////////////////////////////////////////////////////
    // realize image
    ///////////////////////////////////////////////////////////////////////////

    Image<uint16_t> merge_spatial_img(w, h);
    merge_spatial.realize(merge_spatial_img);

    // uncomment following lines to write out intermediate image for testng

    // Func test;
    // test(x, y) = u8(merge_spatial(x, y));
    // Image<uint8_t> test_img(w, h);
    // test.realize(test_img);
    // stbi_write_png("../images/test.png", w, h, 1, test_img.data(), w);
    
    return merge_spatial_img;
}