#include "InputSource.h"

RawImage::RawImage(const std::string &path)
    : RawProcessor(std::make_shared<LibRaw>())
{
    std::cerr << "Opening " << path << std::endl;
    if (int err = RawProcessor->open_file(path.c_str())) {
        std::cerr << "Cannot open file " << path << " error: " << libraw_strerror(err) << std::endl;
        throw std::runtime_error("Error opening " + path);
    }
    if (int err = RawProcessor->unpack())
    {
        std::cerr << "Cannot unpack file " << path << " error: " << libraw_strerror(err) << std::endl;
        throw std::runtime_error("Error opening " + path);
    }
    if (int ret = RawProcessor->raw2image()) {
        std::cerr << "Cannot do raw2image on " << path << " error: " << libraw_strerror(ret) << std::endl;
        throw std::runtime_error("Error opening " + path);
    }

//    RawProcessor->dcraw_ppm_tiff_writer("test.tiff");
//    RawProcessor->imgdata.color.ccm
}

WhiteBalance RawImage::GetWhiteBalance() const {
    const auto coeffs = RawProcessor->imgdata.color.cam_mul;
    // Scale multipliers to green channel
    const float r = coeffs[0] / coeffs[1];
    const float g0 = 1.f; // same as coeffs[1] / coeffs[1];
    const float g1 = 1.f;
    const float b = coeffs[2] / coeffs[1];
    return WhiteBalance{r, g0, g1, b};
}

void RawImage::CopyToBuffer(Halide::Runtime::Buffer<uint16_t> &buffer) const {
    const auto image_data = (uint16_t*)RawProcessor->imgdata.rawdata.raw_image;
    const auto raw_width = RawProcessor->imgdata.rawdata.sizes.raw_width;
    const auto raw_height = RawProcessor->imgdata.rawdata.sizes.raw_height;
    const auto top = RawProcessor->imgdata.rawdata.sizes.top_margin;
    const auto left = RawProcessor->imgdata.rawdata.sizes.left_margin;
    Halide::Runtime::Buffer<uint16_t> raw_buffer(image_data, raw_width, raw_height);
    buffer.copy_from(raw_buffer.translated({-left, -top}));
}

void RawImage::WriteDng(const std::string &path, const Halide::Runtime::Buffer<uint16_t> &buffer) const
{

}

