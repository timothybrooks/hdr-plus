#include "InputSource.h"

RawSource::RawSource(const std::string &path)
        : RawProcessor()
{
    std::cerr << "Processing" << path << std::endl;
    if (int err = RawProcessor.open_file(path.c_str())) {
        std::cerr << "Cannot open file " << path << " error: " << libraw_strerror(err) << std::endl;
    }
    if (int err = RawProcessor.unpack())
    {
        std::cerr << "Cannot unpack file " << path << " error: " << libraw_strerror(err) << std::endl;

    }
    if (int ret = RawProcessor.raw2image()) {
        std::cerr << "Cannot do raw2image on " << path << " error: " << libraw_strerror(ret) << std::endl;
    }
}

WhiteBalance RawSource::GetWhiteBalance() const {
    const auto coeffs = RawProcessor.imgdata.color.cam_mul;
    // Scale multipliers to green channel
    const float r = coeffs[0] / coeffs[1];
    const float g0 = 1.f; // same as coeffs[1] / coeffs[1];
    const float g1 = 1.f;//
    const float b = coeffs[2] / coeffs[1];
    return WhiteBalance{r, g0, g1, b};
}

void RawSource::CopyToBuffer(Halide::Runtime::Buffer<uint16_t> &buffer) const {
    const int data_size = GetWidth() * GetHeight();
    const ushort* const image_data = (ushort*)RawProcessor.imgdata.rawdata.raw_image;
    std::copy(image_data, image_data + data_size, buffer.data() + data_size);
}

