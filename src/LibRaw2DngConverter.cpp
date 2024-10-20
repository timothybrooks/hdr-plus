#include "LibRaw2DngConverter.h"

#include <unordered_map>

#include <libraw/libraw.h>

#include "InputSource.h"

LibRaw2DngConverter::LibRaw2DngConverter(const RawImage &raw)
    : OutputStream(), Raw(raw),
      Tiff(SetTiffFields(
          TiffPtr(TIFFStreamOpen("", &OutputStream), TIFFClose))) {}

LibRaw2DngConverter::TiffPtr
LibRaw2DngConverter::SetTiffFields(LibRaw2DngConverter::TiffPtr tiff_ptr) {
  const auto RawProcessor = Raw.GetRawProcessor();
  const auto raw_color = RawProcessor->imgdata.color;

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

  const std::string cfa = Raw.GetCfaPatternString();
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
      1.f / (raw_color.cam_mul[0] / raw_color.cam_mul[1]), 1.f,
      1.f / (raw_color.cam_mul[2] / raw_color.cam_mul[1])};
  TIFFSetField(tiff, TIFFTAG_ASSHOTNEUTRAL, 3, &as_shot_neutral);

  TIFFSetField(tiff, TIFFTAG_CFALAYOUT, 1); // Rectangular (or square) layout
  TIFFSetField(
      tiff, TIFFTAG_CFAPLANECOLOR, 3,
      "\00\01\02"); // RGB
                    // https://www.awaresystems.be/imaging/tiff/tifftags/cfaplanecolor.html

  const std::array<float, 4> black_level = Raw.GetBlackLevel();
  TIFFSetField(tiff, TIFFTAG_BLACKLEVEL, 4, &black_level);

  static const uint32_t white_level = raw_color.maximum;
  TIFFSetField(tiff, TIFFTAG_WHITELEVEL, 1, &white_level);

  if (RawProcessor->imgdata.sizes.flip > 0) {
    // Seems that LibRaw uses LibTIFF notation.
    TIFFSetField(tiff, TIFFTAG_ORIENTATION, RawProcessor->imgdata.sizes.flip);
  } else {
    TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  }
  return tiff_ptr;
}

void LibRaw2DngConverter::SetBuffer(
    const Halide::Runtime::Buffer<uint16_t> &buffer) const {
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
