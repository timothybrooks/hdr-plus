#include <stdio.h>
#include <iostream>
#include <fstream>

#include <Halide.h>

#include <src/Burst.h>

#include <align_and_merge.h>

Halide::Runtime::Buffer<uint16_t> align_and_merge(Halide::Runtime::Buffer<uint16_t> burst) {
    if (burst.channels() < 2) {
        return {};
    }
    Halide::Runtime::Buffer<uint16_t> merged_buffer(burst.width(), burst.height());
    align_and_merge(burst, merged_buffer);
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

    const auto merged = align_and_merge(burst.ToBuffer());
    std::cerr << "merged size: " << merged.width() << " " << merged.height() << std::endl;

    const RawImage& raw = burst.GetRaw(0);
    const std::string merged_filename = dir_path + "/" + out_name;
    raw.WriteDng(merged_filename, merged);

    return EXIT_SUCCESS;
}
