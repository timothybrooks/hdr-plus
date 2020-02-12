#pragma once

#include <libraw/libraw.h>
#include <sstream>
#include <tiffio.h>
#include <tiffio.hxx>
#include <HalideBuffer.h>

class LibRaw2DngConverter {
public:
    LibRaw2DngConverter(const LibRaw& libraw)
        : OutputStream()
        , Tiff(TIFFStreamOpen("", &OutputStream), [](TIFF* tiff){TIFFClose(tiff);})
    {
        static const uint16_t bayerPatternDimensions[] = {2, 2};
        static const float colorMatrix[] = {
                1.0, 0.0, 0.0,
                0.0, 1.0, 0.0,
                0.0, 0.0, 1.0,
        };
        const auto raw_color = libraw.imgdata.color;
        static const float asShotNeutral[] = {raw_color.cam_mul[0], raw_color.cam_mul[1], raw_color.cam_mul[2]};
        static const uint32_t blackLevel[] = {raw_color.black};
        static const uint32_t whiteLevel[] = {raw_color.maximum};

        const auto tiff = Tiff.get();
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
        TIFFSetField(tiff, TIFFTAG_CFAREPEATPATTERNDIM, bayerPatternDimensions);
        TIFFSetField(tiff, TIFFTAG_CFAPATTERN, "\00\01\01\02");
        TIFFSetField(tiff, TIFFTAG_MAKE, "hdr-plus");
        TIFFSetField(tiff, TIFFTAG_UNIQUECAMERAMODEL, "hdr-plus");
        TIFFSetField(tiff, TIFFTAG_COLORMATRIX1, 9, colorMatrix);
        TIFFSetField(tiff, TIFFTAG_COLORMATRIX1, 9, colorMatrix);
        TIFFSetField(tiff, TIFFTAG_ASSHOTNEUTRAL, 3, asShotNeutral);
        TIFFSetField(tiff, TIFFTAG_CFALAYOUT, 1);
        TIFFSetField(tiff, TIFFTAG_CFAPLANECOLOR, 3, "\00\01\02");
        TIFFSetField(tiff, TIFFTAG_BLACKLEVEL, 1, blackLevel);
        TIFFSetField(tiff, TIFFTAG_WHITELEVEL, 1, whiteLevel);
    }

    void SetBuffer(const Halide::Runtime::Buffer<uint16_t>& buffer) const {
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

    void Write(const std::string& path) const {
        TIFFFlush(Tiff.get());
        std::ofstream output(path, std::ofstream::binary);
        output << OutputStream.str();
    }

private:
    std::ostringstream OutputStream;
    std::shared_ptr<TIFF> Tiff;
};