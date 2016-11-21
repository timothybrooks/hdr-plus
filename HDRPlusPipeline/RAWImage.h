#ifndef RAW_IMAGE_H
#define RAW_IMAGE_H

#include <exception>

#include "Image.h"
#include "Halide.h"

#define VAR_WINDOW_RATIO

class RAWImage
{

private:
    unsigned char *p;
    size_t w, h;
    const static size_t c = 1;

public:       

    RAWImage()
    {
        w = h = 0;
        p = NULL;
    }

    RAWImage(size_t width, size_t height, double *pixels) : w(width), h(height), p(pixels) {}

    RAWImage(std::string filename) {
        read(filename);
    }

    inline size_t width() {
        return w;
    }

    inline size_t height() {
        return h;
    }

    inline unsigned char* pixels() {
        return p;
    }

    inline unsigned char pixel(size_t x, size_t y) {
        if (x < 0 || x >= w || y < 0 || y >= h) {
            throw std::out_of_range("Tried accessing a pixel out of the image boundaries");
        }
        return p[y * w + x];
    }
    //estimate of contrast based on Welford's algorithm for approximating variance
    double variance(size_t windowWidth = width / VAR_WINDOW_RATIO, size_t windowHeight = height / VAR_WINDOW_RATIO);
    void read(std::string filename);
    void write(std::string filename);
    void makePyramid();
    Image demosaic();
};

#endif