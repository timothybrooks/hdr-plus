#pragma once

#include <string>

#include <libraw/libraw.h>

#include <Halide.h>
#include "finish.h"

class AbstractInputSource {
public:
    virtual ~AbstractInputSource() = default;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual int GetBlackLevel() const = 0;
    virtual int GetWhiteLevel() const = 0;
    virtual WhiteBalance GetWhiteBalance() const = 0;
    virtual void CopyToBuffer(Halide::Runtime::Buffer<uint16_t>& buffer) const = 0;

    using SPtr = std::shared_ptr<AbstractInputSource>;
};

class RawSource : public AbstractInputSource {
public:
    explicit RawSource(const std::string& path);

    ~RawSource() override {
        RawProcessor.free_image();
    }

    int GetWidth() const override { return RawProcessor.imgdata.sizes.raw_width; }

    int GetHeight() const override { return RawProcessor.imgdata.sizes.raw_height; }

    int GetBlackLevel() const override { return RawProcessor.imgdata.color.black; }

    int GetWhiteLevel() const override { return RawProcessor.imgdata.color.maximum; }

    WhiteBalance GetWhiteBalance() const override;

    void CopyToBuffer(Halide::Runtime::Buffer<uint16_t>& buffer) const override;

private:
    LibRaw RawProcessor;
};
