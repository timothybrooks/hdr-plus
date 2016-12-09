#include "merge.h"

#include "Halide.h"

#define PI 3.141592f
#include "../include/stb_image_write.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

Expr tile_0(Expr e) {
    return e / T_SIZE_2 - 1;
}

Expr tile_1(Expr e) {
    return e / T_SIZE_2;
}

Expr idx_0(Expr e) {
    return e % T_SIZE_2 + T_SIZE_2;
}

Expr idx_1(Expr e) {
    return e % T_SIZE_2;
}

Expr idx_im(Expr t, Expr i) {
    return t * T_SIZE_2 + i;
}

//alignment(isYOffset, tx, ty, n)

Image<uint16_t> merge(Image<uint16_t> imgs, Func alignment) {

    // TODO: Weiner filtering or something like that...

    int w = imgs.width();
    int h = imgs.height();

    //std::cout << "test: " << (-1 % 2) << std::endl;
    //int num_imgs = imgs.extent(2);

    Func merge_temporal("merge_temporal");
    Var ix, iy, tx, ty;

    //RDom r(0, num_imgs);

    //merge_temporal(ix, iy, tx, ty) = u16(sum(u32(imgs(idx_im(tx, ix), idx_im(ty, iy), r))) / num_imgs);

    // edge cases

    //merge_temporal(ix, iy, -1, ty) = merge_temporal(ix - T_SIZE_2, iy - T_SIZE_2, 0, ty);
    //merge_temporal(ix, iy, tx, -1) = merge_temporal(ix - T_SIZE_2, iy - T_SIZE_2, tx, 0);

    //merge_temporal(ix, iy, w, ty) = merge_temporal(ix + T_SIZE_2, iy - T_SIZE_2, w - 1, ty);
    //merge_temporal(ix, iy, tx, h) = merge_temporal(ix + T_SIZE_2, iy - T_SIZE_2, tx, h - 1);

    merge_temporal(ix, iy, tx, ty) = select((tx + ty) % 2 == 0, 0, 255);

    ///////////////////////////////////////////////////////////////////////////
    // spatial merging
    ///////////////////////////////////////////////////////////////////////////

    // coefficient (raised cosine window)

    Func coef("coef_raised_cosine");
    Var n;

    coef(n) = 0.5f - 0.5f * cos(2 * PI * (n + 0.5f) / T_SIZE);

    // tile coefficient expressions

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

    // schedule

    coef.compute_root()
        .vectorize(n, 16);

    // TODO: Not sure why Halide throws an error when I try scheduling here...
    //merge_spatial.compute_root()
    //             .parallel(y)
    //             .vectorize(x);

    // realize image

    Image<uint16_t> merge_spatial_img(w, h);
    merge_spatial.realize(merge_spatial_img);

    Func test;
    test(x, y) = u8(merge_spatial(x, y));
    Image<uint8_t> test_img(w, h);
    test.realize(test_img);


    stbi_write_png("../images/test.png", w, h, 1, test_img.data(), w);
    
    return merge_spatial_img;
}