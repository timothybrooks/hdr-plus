#pragma once

#include <array>
#include <string>

#include <libraw/libraw.h>

#include "finish.h"
#include <Halide.h>

class RawImage {
public:
  explicit RawImage(const std::string &path);

  ~RawImage() = default;

  int GetWidth() const { return RawProcessor->imgdata.rawdata.sizes.width; }

  int GetHeight() const { return RawProcessor->imgdata.rawdata.sizes.height; }

  int GetScalarBlackLevel() const;

  std::array<float, 4> GetBlackLevel() const;

  int GetWhiteLevel() const { return RawProcessor->imgdata.color.maximum; }

  WhiteBalance GetWhiteBalance() const;

  std::string GetCfaPatternString() const;
  CfaPattern GetCfaPattern() const;

  Halide::Runtime::Buffer<float> GetColorCorrectionMatrix() const;

  void CopyToBuffer(Halide::Runtime::Buffer<uint16_t> &buffer) const;

  // Writes current RawImage as DNG. If buffer was provided, then use it instead
  // of internal buffer.
  void WriteDng(const std::string &path,
                const Halide::Runtime::Buffer<uint16_t> &buffer = {}) const;

  std::shared_ptr<LibRaw> GetRawProcessor() const { return RawProcessor; }

private:
  std::string Path;
  std::shared_ptr<LibRaw> RawProcessor;
};
