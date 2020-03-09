#include "InputSource.h"

#include <algorithm>
#include <unordered_map>

#include "LibRaw2DngConverter.h"

RawImage::RawImage(const std::string &path)
    : Path(path)
    , RawProcessor(std::make_shared<LibRaw>())
{
//    TODO: Check LibRaw parametres.
//    RawProcessor->imgdata.params.X = Y;

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

void RawImage::WriteDng(const std::string &output_path, const Halide::Runtime::Buffer<uint16_t> &buffer) const
{
    LibRaw2DngConverter converter(*this);
    converter.SetBuffer(buffer);
    converter.Write(output_path);
}

std::array<float, 4> RawImage::GetBlackLevel() const {
    // See https://www.libraw.org/node/2471
    const auto raw_color = RawProcessor->imgdata.color;
    const auto base_black_level = static_cast<float>(raw_color.black);

    std::array<float, 4> black_level = {
        base_black_level + static_cast<float>(raw_color.cblack[0]),
        base_black_level + static_cast<float>(raw_color.cblack[1]),
        base_black_level + static_cast<float>(raw_color.cblack[2]),
        base_black_level + static_cast<float>(raw_color.cblack[3])
    };

    if (raw_color.cblack[4] == 2 && raw_color.cblack[5] == 2) {
        for (int x = 0; x < raw_color.cblack[4]; ++x) {
            for (int y = 0; y < raw_color.cblack[5]; ++y) {
                const auto index = y * 2 + x;
                black_level[index] = raw_color.cblack[6 + index];
            }
        }
    }

    return black_level;
}

int RawImage::GetScalarBlackLevel() const {
    const auto black_level = GetBlackLevel();
    return static_cast<int>(*std::min_element(black_level.begin(), black_level.end()));
}

std::string RawImage::GetCfaPatternString() const {
    static const std::unordered_map<char, char> CDESC_TO_CFA = {
        {'R', 0},
        {'G', 1},
        {'B', 2},
        {'r', 0},
        {'g', 1},
        {'b', 2}
    };
    const auto &cdesc = RawProcessor->imgdata.idata.cdesc;
    return {
        CDESC_TO_CFA.at(cdesc[RawProcessor->COLOR(0, 0)]),
        CDESC_TO_CFA.at(cdesc[RawProcessor->COLOR(0, 1)]),
        CDESC_TO_CFA.at(cdesc[RawProcessor->COLOR(1, 0)]),
        CDESC_TO_CFA.at(cdesc[RawProcessor->COLOR(1, 1)])
    };
}

CfaPattern RawImage::GetCfaPattern() const {
    const auto cfa_pattern = GetCfaPatternString();
    if (cfa_pattern == std::string{0, 1, 1, 2}) {
        return CfaPattern::CFA_RGGB;
    } else if (cfa_pattern == std::string{1, 0, 2, 1}) {
        return CfaPattern::CFA_GRBG;
    } else if (cfa_pattern == std::string{2, 1, 1, 0}) {
        return CfaPattern::CFA_BGGR;
    } else if (cfa_pattern == std::string{1, 2, 0, 1}) {
        return CfaPattern::CFA_GBRG;
    }
    throw std::invalid_argument("Unsupported CFA pattern: " + cfa_pattern);
    return CfaPattern::CFA_UNKNOWN;
}

Halide::Runtime::Buffer<float> RawImage::GetColorCorrectionMatrix() const {
    const auto raw_color = RawProcessor->imgdata.color;
    Halide::Runtime::Buffer<float> ccm(3, 3);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            ccm(i, j) = raw_color.rgb_cam[j][i];
        }
    }
    return ccm;
}


