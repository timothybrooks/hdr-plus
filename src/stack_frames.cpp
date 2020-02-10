#include "Halide.h"
#include <halide_image_io.h>
#include "halide_load_raw.h"

#include "Burst.h"

#include <stdio.h>
#include "align.h"
#include "merge.h"
#include "finish.h"
#include <iostream>
#include <fstream>

#include <raw2dng/raw2dng.h>

using namespace Halide;
using namespace std;

Halide::Buffer<uint16_t> align_and_merge(const Halide::Runtime::Buffer<uint16_t>& burst) {
    if (burst.channels() < 2) {
        return {};
    }

    Halide::Buffer<uint16_t> imgsBuffer(*burst.raw_buffer());

    Func alignment = align(imgsBuffer);
    Func merged = merge(imgsBuffer, alignment);

    Halide::Buffer<uint16_t> merged_buffer(burst.width(), burst.height());
    merged.realize(merged_buffer);

    return merged_buffer;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " dir_path out_img raw_img1 raw_img2 [...]" << std::endl;
        return 1;
    }

    int i = 1;
    if (argc - i < 3) {
        std::cerr << "Usage: " << argv[0] << " dir_path out_img raw_img1 raw_img2 [...]" << std::endl;
        return 1;
    }

    const std::string dir_path = argv[i++];
    const std::string out_name = argv[i++];

    std::vector<std::string> in_names;
    while (i < argc) in_names.push_back(argv[i++]);

    Burst burst(dir_path, in_names);

    Halide::Buffer<uint16_t> merged = align_and_merge(burst.ToBuffer());
    std::cerr << "merged size: " << merged.width() << " " << merged.height() << std::endl;

    const std::string merged_filename = dir_path + "/merged.png";
    Halide::Tools::save_image(merged, merged_filename);

    // Get 0 raw and use it as recipient for merged buffer
//    RawImage raw = burst.GetRaw(0);
//    const std::string merged_filename = dir_path + "/" + out_name;
//    raw.WriteDng(merged_filename, merged);
    raw2dng(dir_path + "/" + in_names[0], "test.dng", "", false);

    return EXIT_SUCCESS;
}
