#ifndef RAW_IMAGE_H
#define RAW_IMAGE_H

#include <exception>

#include "Image.h"
#include "Halide.h"

#define VAR_WINDOW_RATIO

class RAWImage
{

private:
    const static size_t c = 1;
    Halide::Image<float> pyrLayer0;
    Halide::Image<float> pyrLayer1;
    Halide::Image<float> pyrLayer2;
    Halide::Image<float> pyrLayer3;
    size_t w, h;
    unsigned char *p;

public:       

    RAWImage()
    {
        w = h = 0;
        p = NULL;
    }

    RAWImage(size_t width, size_t height, unsigned char *pixels) : w(width), h(height), p(pixels) {}

    RAWImage(std::string filename) {
        read(filename);
    }

    inline size_t width() {
        return w;
    }

    inline size_t height() {
        return h;
    }

    inline unsigned char pixel(size_t x, size_t y) {
        if (x < 0 || x >= w || y < 0 || y >= h) {
            throw std::out_of_range("Tried accessing a pixel out of the image boundaries");
        }
        return p[y * w + x];
    }
    //estimate of contrast based on Welford's algorithm for approximating variance
    double variance(size_t windowWidth = 1, size_t windowHeight = 1);
    void read(std::string filename);
    void write(std::string filename);
    void makePyramid();
    Image demosaic();
};

#endif