#include "RAWImage.h"

#include <exception>

#include "include/stb_image.h"
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
    size_t startX = this->width() - windowWidth / 2;
    size_t startY = this->height() - windowHeight / 2;
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
        throw std::runtime_error("STBI Error. Cannot write file '" + filename + "'");
    }
}

void RAWImage::makePyramid() {

    int width = this->width();
    int height = this->height();

    Halide::Var x, y;
    Halide::Func origin;
    origin(x,y) = 0.f;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            origin(col,row) = this->pixel(col,row);
        }
    }

    int stride0 = 2;
    int stride1 = 2;
    int stride2 = 4;
    int stride3 = 4;

    int layer0Width = this->width()/stride0;
    int layer0Height = this->height()/stride0;
    int layer1Width = layer0Width/stride1;
    int layer1Height = layer0Height/stride1;
    int layer2Width = layer1Width/stride2;
    int layer2Height = layer1Height/stride2;
    int layer3Width = layer2Width/stride3;
    int layer3Height = layer2Height/stride3;

    if (layer3Width == 0 || layer3Height == 0) {
        throw std::runtime_error("Input images too small for pyramid creation.");
    }

    Halide::Func layer0;
    Halide::Func layer1;
    Halide::Func layer2;
    Halide::Func layer3;
    layer0(x,y) = 0.f;
    layer1(x,y) = 0.f;
    layer2(x,y) = 0.f;
    layer3(x,y) = 0.f;


    Halide::RDom r0(0, stride0, 0, stride0);
    layer0(x,y) += origin(x * stride0 + r0.x, y * stride0 + r0.y);
    Halide::RDom r1(0,stride1,0,stride1);
    layer1(x,y) += layer0(x * stride1 + r1.x, y * stride1 + r1.y);
    Halide::RDom r2(0,stride2,0,stride2);
    layer2(x,y) += layer1(x * stride2 + r2.x, y * stride2 + r2.y);    
    Halide::RDom r3(0,stride3,0,stride3);
    layer3(x,y) += layer2(x * stride3 + r3.x, y * stride3 + r3.y);

    this->pyrLayer0 = layer0.realize(layer0Width, layer0Height);
    this->pyrLayer1 = layer1.realize(layer1Width, layer1Height);
    this->pyrLayer2 = layer2.realize(layer2Width, layer2Height);
    this->pyrLayer3 = layer3.realize(layer3Width, layer3Height);
}

Image RAWImage::demosaic() {
    return Image();
}