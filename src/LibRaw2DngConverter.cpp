#include "LibRaw2DngConverter.h"

#include <unordered_map>

namespace {
    std::array<float, 4> GetBlackLevel(const libraw_colordata_t &raw_color) {
        // See https://www.libraw.org/node/2471
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

    std::string GetCfaPattern(const LibRaw &raw) {
        static const std::unordered_map<char, char> CDESC_TO_CFA = {
            {'R', 0},
            {'G', 1},
            {'B', 2},
            {'r', 0},
            {'g', 1},
            {'b', 2}
        };
        const auto &cdesc = raw.imgdata.idata.cdesc;
        auto &mutable_raw = const_cast<LibRaw &>(raw); // LibRaw is not const correct :-(
        return {
            CDESC_TO_CFA.at(cdesc[mutable_raw.COLOR(0, 0)]),
            CDESC_TO_CFA.at(cdesc[mutable_raw.COLOR(0, 1)]),
            CDESC_TO_CFA.at(cdesc[mutable_raw.COLOR(1, 0)]),
            CDESC_TO_CFA.at(cdesc[mutable_raw.COLOR(1, 1)])
        };
    }
}

LibRaw2DngConverter::LibRaw2DngConverter(const LibRaw &raw)
    : OutputStream()
    , Raw(raw)
    , RawIsDng(Raw.imgdata.idata.dng_version != 0)
    , Tiff(SetTiffFields(TiffPtr(TIFFStreamOpen("", &OutputStream), TIFFClose)))
{}

LibRaw2DngConverter::TiffPtr LibRaw2DngConverter::SetTiffFields(LibRaw2DngConverter::TiffPtr tiff_ptr) {
    const auto dng_version = Raw.imgdata.idata.dng_version;
    const bool is_dng = dng_version != 0;

    const auto raw_color = Raw.imgdata.color;

    const uint16_t bayer_pattern_dimensions[] = {2, 2};

    const auto tiff = tiff_ptr.get();
    TIFFSetField(tiff, TIFFTAG_DNGVERSION, "\01\04\00\00");
    TIFFSetField(tiff, TIFFTAG_DNGBACKWARDVERSION, "\01\04\00\00");
    TIFFSetField(tiff, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 16);
    TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_CFA);
    TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tiff, TIFFTAG_CFAREPEATPATTERNDIM, &bayer_pattern_dimensions);

    const std::string cfa = GetCfaPattern(Raw);
    TIFFSetField(tiff, TIFFTAG_CFAPATTERN, cfa.c_str());

    TIFFSetField(tiff, TIFFTAG_MAKE, "hdr-plus");
    TIFFSetField(tiff, TIFFTAG_UNIQUECAMERAMODEL, "hdr-plus");

    const std::array<float, 9> color_matrix = {
        raw_color.cam_xyz[0][0], raw_color.cam_xyz[0][1], raw_color.cam_xyz[0][2],
        raw_color.cam_xyz[1][0], raw_color.cam_xyz[1][1], raw_color.cam_xyz[1][2],
        raw_color.cam_xyz[2][0], raw_color.cam_xyz[2][1], raw_color.cam_xyz[2][2],
    };
    TIFFSetField(tiff, TIFFTAG_COLORMATRIX1, 9, &color_matrix);
    TIFFSetField(tiff, TIFFTAG_CALIBRATIONILLUMINANT1, 21); // D65

    const std::array<float, 3> as_shot_neutral = {
        1.f / (raw_color.cam_mul[0] / raw_color.cam_mul[1]),
        1.f,
        1.f / (raw_color.cam_mul[2] / raw_color.cam_mul[1])
    };
    TIFFSetField(tiff, TIFFTAG_ASSHOTNEUTRAL, 3, &as_shot_neutral);

    TIFFSetField(tiff, TIFFTAG_CFALAYOUT, 1); // Rectangular (or square) layout
    TIFFSetField(tiff, TIFFTAG_CFAPLANECOLOR, 3, "\00\01\02"); // RGB https://www.awaresystems.be/imaging/tiff/tifftags/cfaplanecolor.html

    const std::array<float, 4> black_level = GetBlackLevel(raw_color);
    TIFFSetField(tiff, TIFFTAG_BLACKLEVEL, 4, &black_level);

    static const uint32_t white_level = raw_color.maximum;
    TIFFSetField(tiff, TIFFTAG_WHITELEVEL, 1, &white_level);
    
    if (Raw.imgdata.sizes.flip > 0) {
        // Seems that LibRaw uses LibTIFF notation.
        TIFFSetField(tiff, TIFFTAG_ORIENTATION, Raw.imgdata.sizes.flip);
    } else {
        TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    }
    return tiff_ptr;
}

void LibRaw2DngConverter::SetBuffer(const Halide::Runtime::Buffer<uint16_t> &buffer) const {
    const auto width = buffer.width();
    const auto height = buffer.height();
    const auto tiff = Tiff.get();
    TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height);

    uint16_t *row_pointer = buffer.data();
    for (int row = 0; row < height; row++) {
        TIFFWriteScanline(tiff, row_pointer, row, 0);
        row_pointer += width;
    }
}

void LibRaw2DngConverter::Write(const std::string &path) const {
    TIFFCheckpointDirectory(Tiff.get());
    TIFFFlush(Tiff.get());
    std::ofstream output(path, std::ofstream::binary);
    output << OutputStream.str();
}
