#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <stdexcept>

class Image
{

private:
    size_t w, h;
    unsigned char *p;
    const static size_t c = 3;

    void toneMap();

public:       

    Image()
    {
        w = h = 0;
        p = NULL;
    }

    Image(size_t width, size_t height, unsigned char *pixels) : w(width), h(height), p(pixels) {}

    Image(std::string filename) {
        read(filename);
    }

    inline size_t width() {
        return w;
    }

    inline size_t height() {
        return h;
    }

    inline unsigned char pixel(size_t x, size_t y, size_t z) {
        if (x < 0 || x >= w || y < 0 || y >= h || z < 0 || z >= c) {
            throw std::out_of_range("Tried accessing a pixel out of the image boundaries");
        }
        return p[c * (y * w + x) + z];
    }

    void read(std::string filename);
    void write(std::string filename);
    void finish();
};

#endif