#pragma once

#include "InputSource.h"

#include <Halide.h>

#include <string>
#include <vector>
#include <Halide.h>

class Burst : public AbstractInputSource {
public:
    Burst(std::string dir_path, std::vector<std::string> inputs)
        : Dir(std::move(dir_path))
        , Inputs(std::move(inputs))
        , Raws(LoadRaws(Dir, Inputs))
    {}

    ~Burst() override = default;

    int GetWidth() const override { return Raws.empty() ? -1 : Raws[0]->GetWidth(); }

    int GetHeight() const override { return Raws.empty() ? -1 : Raws[0]->GetHeight(); }

    int GetBlackLevel() const override { return Raws.empty() ? -1 : Raws[0]->GetBlackLevel(); }

    int GetWhiteLevel() const override { return Raws.empty() ? -1 : Raws[0]->GetWhiteLevel(); }

    WhiteBalance GetWhiteBalance() const override { return Raws.empty() ? WhiteBalance{-1, -1, -1, -1} : Raws[0]->GetWhiteBalance(); }

    Halide::Runtime::Buffer<uint16_t> ToBuffer() const;

    void CopyToBuffer(Halide::Runtime::Buffer<uint16_t>& buffer) const override;

private:
    std::string Dir;
    std::vector<std::string> Inputs;
    std::vector<AbstractInputSource::SPtr> Raws;

private:
    static std::vector<AbstractInputSource::SPtr> LoadRaws(const std::string& dirPath, std::vector<std::string>& inputs);
};
