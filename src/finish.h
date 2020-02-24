#ifndef HDRPLUS_FINISH_H_
#define HDRPLUS_FINISH_H_

#include "Halide.h"
     
template <class T = float>
struct TypedWhiteBalance {
    template<class TT>
    TypedWhiteBalance(const TypedWhiteBalance<TT>& other)
        : r(other.r)
        , g0(other.g0)
        , g1(other.g1)
        , b(other.b)
    {}

    TypedWhiteBalance(T r, T g0, T g1, T b)
        : r(r)
        , g0(g0)
        , g1(g1)
        , b(b)
    {}
    
    T r;
    T g0;
    T g1;
    T b;
};
using WhiteBalance = TypedWhiteBalance<float>;
using CompiletimeWhiteBalance = TypedWhiteBalance<Halide::Expr>;

typedef uint16_t BlackPoint;
typedef uint16_t WhitePoint;

typedef float Compression;
typedef float Gain;

/*
 * finish -- Applies a series of standard local and global image processing
 * operations to an input mosaicked image, producing a pleasant color output.
 * Input pecifies black-level, white-level and white balance. Additionally,
 * tone mapping is applied to the image, as specified by the input compression
 * and gain amounts. This produces natural-looking brightened shadows, without
 * blowing out highlights. The output values are 8-bit.
 */
Halide::Func finish(Halide::Func input, int width, int height, const BlackPoint bp, const WhitePoint wp, const WhiteBalance &wb, const Compression c, const Gain g);
Halide::Func finish(Halide::Func input, Halide::Expr width, Halide::Expr height, const Halide::Expr bp, const Halide::Expr wp, const CompiletimeWhiteBalance &wb, const Halide::Expr c, const Halide::Expr g);

#endif