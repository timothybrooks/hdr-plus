#pragma once

#include <libraw/libraw.h>
#include <sstream>
#include <tiffio.h>
#include <tiffio.hxx>
#include <HalideBuffer.h>

class LibRaw2DngConverter {
    using TiffPtr = std::shared_ptr<TIFF>;
    TiffPtr SetTiffFields(TiffPtr tiff_ptr);
public:
    explicit LibRaw2DngConverter(const LibRaw& raw);

    void SetBuffer(const Halide::Runtime::Buffer<uint16_t>& buffer) const;

    void Write(const std::string& path) const;

private:
    std::ostringstream OutputStream;
    const LibRaw& Raw;
    bool RawIsDng;
    std::shared_ptr<TIFF> Tiff;
};
