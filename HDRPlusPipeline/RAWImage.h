#ifndef RAW_IMAGE_H
#define RAW_IMAGE_H

#include <string>
#include <stdexcept>

#include "Image.h"
#include "Halide.h"

class RAWImage
{

private:
    const static size_t c = 1;
    size_t w, h;
    unsigned char *p;

    Halide::Image<float> pyrLayer0;
    Halide::Image<float> pyrLayer1;
    Halide::Image<float> pyrLayer2;
    Halide::Image<float> pyrLayer3;

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
            throw std::out_of_range("Tried accessing a RAWImage pixel out of the image boundaries");
        }
        return p[y * w + x];
    }
    
    void read(std::string filename);
    void write(std::string filename);

    double variance();
    void makePyramid();
    Image demosaic();
};

#endif