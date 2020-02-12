#pragma once

#include <string>

#include <libraw/libraw.h>

#include <Halide.h>
#include "finish.h"

class RawImage {
public:
    explicit RawImage(const std::string& path);

    ~RawImage() = default;

    int GetWidth() const { return RawProcessor->imgdata.rawdata.sizes.width; }

    int GetHeight() const { return RawProcessor->imgdata.rawdata.sizes.height; }

    int GetBlackLevel() const { return RawProcessor->imgdata.color.black; }

    int GetWhiteLevel() const { return RawProcessor->imgdata.color.maximum; }

    WhiteBalance GetWhiteBalance() const;

    void CopyToBuffer(Halide::Runtime::Buffer<uint16_t>& buffer) const;

    // Writes current RawImage as DNG. If buffer was provided, then use it instead of internal buffer.
    void WriteDng(const std::string& path, const Halide::Runtime::Buffer<uint16_t>& buffer = {}) const;

private:
    std::string Path;
    std::shared_ptr<LibRaw> RawProcessor;
};
