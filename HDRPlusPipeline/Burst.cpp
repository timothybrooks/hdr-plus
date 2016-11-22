#include "Burst.h"

void Burst::align(void) {
    return;
}

Burst::Burst(std::string name, std::vector<std::string> imFileNames) : name(name), burstLength(imFileNames.size()) {
    if (this->burstLength < 2) {
        throw std::runtime_error("too few images in burst");
    } else if (this->burstLength > MAX_BURST_LENGTH) {
        std::cout << "Too many images in burst. Ignoring beyond the first " << MAX_BURST_LENGTH << " frames." << std::endl;
        this->burstLength = MAX_BURST_LENGTH;
    }
    this->alternates.resize(this->burstLength-1);
    this->reference = new RAWImage(imFileNames[0]);
    this->w = this->reference->width();
    this->h = this->reference->height();
    float bestVariance = this->reference->variance();
    RAWImage* currRAWImage = NULL;
    float currVariance;
    
    //TODO profile this to see whether it could be improved with parallelism. Probably bandwidth-bound
    for (size_t i = 1; i < std::min(this->burstLength,(size_t)3); i++) {
        currRAWImage = new RAWImage(imFileNames[i]);
        if (currRAWImage->width() != this->width() || currRAWImage->height() != this->height()) {
            throw std::runtime_error("RAWImage dimensions in burst do not match");
        }
        currVariance = currRAWImage->variance();
        if (currVariance > bestVariance) {
            this->alternates[i-1] = this->reference;
            bestVariance = currVariance;
            this->reference = currRAWImage;
        } else {
            this->alternates[i-1] = currRAWImage;
        }
    }
    //only consider first three frames as potential reference
    for (size_t i = 3; i < this->burstLength; i++) {
        this->alternates[i-1] = new RAWImage(imFileNames[i]);
    }
}

RAWImage Burst::merge(void) {
    return RAWImage();
}