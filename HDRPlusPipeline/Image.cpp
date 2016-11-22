#include "Image.h"
#include <exception>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "./include/stb_image.h"
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./include/stb_image_write.h"
#endif

void Image::read(std::string filename) {
    int _w, _h, _num_input_channels; // We do not care about the number of input channels

    p = stbi_load(filename.c_str(), &_w, &_h, &_num_input_channels, c);
    w = _w; h = _h; // int* type must be passed to stbi_load

    if (!p) {
        throw std::runtime_error("STBI Error: " + (std::string)(stbi_failure_reason()) + ". Cannot read file '" + filename + "'");
    }
}

void Image::write(std::string filename) {
    if(!stbi_write_png(filename.c_str(), w, h, c, p, w*c)) {
        throw std::runtime_error("STBI Error: " + (std::string)(stbi_failure_reason()) + ". Cannot write file '" + filename + "'");
    }
}

void Image::finish() {
    toneMap();
}

void Image::toneMap() {
    
}