#ifndef HDRPLUS_FINISH_H_
#define HDRPLUS_FINISH_H_

#include "Halide.h"
     
template <class T = float>
struct TypedWhiteBalance {
    template<class TT>
    explicit TypedWhiteBalance(const TypedWhiteBalance<TT>& other)
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

enum class CfaPattern : int {
    CFA_UNKNOWN = 0,
    CFA_RGGB = 1,
    CFA_GRBG = 2,
    CFA_BGGR = 3,
    CFA_GBRG = 4
};

/*
 * finish -- Applies a series of standard local and global image processing
 * operations to an input mosaicked image, producing a pleasant color output.
 * Input pecifies black-level, white-level and white balance. Additionally,
 * tone mapping is applied to the image, as specified by the input compression
 * and gain amounts. This produces natural-looking brightened shadows, without
 * blowing out highlights. The output values are 8-bit.
 */
Halide::Func finish(Halide::Func input, int width, int height, BlackPoint bp, WhitePoint wp, const WhiteBalance &wb, CfaPattern cfa, Halide::Func ccm, Compression c, Gain g);
Halide::Func finish(Halide::Func input, Halide::Expr width, Halide::Expr height, Halide::Expr bp, Halide::Expr wp, const CompiletimeWhiteBalance &wb, Halide::Expr cfa_pattern, Halide::Func ccm, Halide::Expr c, Halide::Expr g);

#endif