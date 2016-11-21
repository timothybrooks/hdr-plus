#include "RAWImage.h"

{
    double variance(size_t windowWidth, size_t windowHeight) {
        if (windowWidth > this->width() || windowHeight > this->height()) {
            throw std::exception("Invalid variance window size");
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

    }

    void RAWImage::write(std::string filename) {
        
    }

    Image RAWImage::demosaic() {
        
    }
}