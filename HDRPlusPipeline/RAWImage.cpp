#include "RAWImage.h"
#include <exception>

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

double RAWImage::variance(size_t windowWidth, size_t windowHeight) {
    if (windowWidth > this->width() || windowHeight > this->height()) {
        throw std::runtime_error("Invalid variance window size");
    }
    double avg = 0.0;
    double M = 0.0;
    double delta;
    double pixel;
    uint32_t n = 0;
    size_t startX = this->width - windowWidth / 2;
    size_t startY = this->height - windowHeight / 2;
    for (size_t y = startY; y < startY + windowHeight; y++) {
        for (size_t x = startX; x < startX + windowWidth; x++) {
            n++;
            pixel = this->pixel(x,y);
            delta = (float) (pixel - avg);
            avg += delta/n;
            M += delta*(pixel-avg);
        }
    }
    return M / (n-1);
}

void RAWImage::read(std::string filename) {
    int _w, _h, _num_input_channels; // We do not care about the number of input channels

    p = stbi_load(filename.c_str(), &_w, &_h, &_num_input_channels, c);
    w = _w; h = _h; // int* type must be passed to stbi_load

    if (!p) {
        throw std::runtime_error("STBI Error: " + (std::string)(stbi_failure_reason()) + ". Cannot read file '" + filename + "'");
    }
}

void RAWImage::write(std::string filename) {
    if(!stbi_write_png(filename.c_str(), w, h, c, p, w*c)) {
        throw std::runtime_error("STBI Error: " + (std::string)(stbi_failure_reason()) + ". Cannot write file '" + filename + "'");
    }
}

void RAWImage::makePyramid() {
    //Downsampled bayer to greyscale
    Halide::Func layer0;
    layer0(x,y) = ((double)this->pixel(x/2,y/2) + (double)this->pixel(x/2 + 1, y/2) + (double)this->pixel(x/2, y/2 + 1) + (double)this->pixel(x/2 + 1, y/2 + 1)) / 4.0;
    this->pyrLayer0 = layer0.realize(this->width(), this->height());

    Halide::Image<float> pyrLayer1;
    this->pyrLayer1(x,y) = (pyrLayer0(x/2,y/2) + pyrLayer0(x/2 + 1, y/2) + pyrLayer0(x/2, y/2 + 1) + pyrLayer0(x/2 + 1, y/2 + 1)) / 4.0;

    Halide::Image<float> pyrLayer2;
    this->pyrLayer2(x,y) = (pyrLayer1(x/2,y/2) + pyrLayer1(x/2 + 1, y/2) + pyrLayer1(x/2, y/2 + 1) + pyrLayer1(x/2 + 1, y/2 + 1)) / 4.0;

    Halide::Image<float> pyrLayer3;
    this->pyrLayer3(x,y) = (pyrLayer2(x/2,y/2) + pyrLayer2(x/2 + 1, y/2) + pyrLayer2(x/2, y/2 + 1) + pyrLayer2(x/2 + 1, y/2 + 1)) / 4.0;
}

Image RAWImage::demosaic() {
    return Image();
}