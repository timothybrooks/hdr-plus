#include "Image.h"
#include <exception>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

void Image::read(std::string filename) {
    int _w, _h, num_input_channels;

    p = stbi_load(filename.c_str(), &_w, &_h, &num_input_channels, c);
    w = _w; h = _h;

    if (!p) {
        throw std::runtime_error("Cannot read file '" + filename + "'");
    }
    std::cout << "w: " << w << ", h: " << h << std::endl;
}

void Image::write(std::string filename) {
    int stride_in_bytes = 1;
    if(!stbi_write_png(filename.c_str(), w, h, c, p, w*c)) {
        throw std::runtime_error("Cannot write file '" + filename + "'");
    }
}

void Image::finish() {
    toneMap();
}

void Image::toneMap() {
    
}