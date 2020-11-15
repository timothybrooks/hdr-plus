#include <iostream>
#include <fstream>
#include <stdio.h>

#include <Halide.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <include/stb_image_write.h>

#include <src/Burst.h>
#include <hdrplus_pipeline.h>

/*
 * HDRPlus Class -- Houses file I/O, defines pipeline attributes and calls
 * processes main stages of the pipeline.
 */
class HDRPlus {
    const Burst& burst;
public:
    const Compression c;
    const Gain g;
    
    HDRPlus(Burst burst, const Compression  c, const Gain g)
        : burst(burst)
        , c(c)
        , g(g)
    {
    }
    
    Halide::Runtime::Buffer<uint8_t> process() {
        const int width = burst.GetWidth();
        const int height = burst.GetHeight();

        Halide::Runtime::Buffer<uint8_t> output_img(3, width, height);

        std::cerr << "Black point: " << burst.GetBlackLevel() << std::endl;
        std::cerr << "White point: " << burst.GetWhiteLevel() << std::endl;
        
        const WhiteBalance wb = burst.GetWhiteBalance();
        std::cerr << "RGGB: " << wb.r << " " << wb.g0 << " " << wb.g1 << " " << wb.b << std::endl;

        Halide::Runtime::Buffer<uint16_t> imgs = burst.ToBuffer();
        if (imgs.dimensions() != 3 || imgs.extent(2) < 2) {
            throw std::invalid_argument("The input of HDRPlus must be a 3-dimensional buffer with at least two channels.");
        }

        const int cfa_pattern = static_cast<int>(burst.GetCfaPattern());
        auto ccm = burst.GetColorCorrectionMatrix();
        hdrplus_pipeline(imgs, burst.GetBlackLevel(), burst.GetWhiteLevel(), wb.r, wb.g0, wb.g1, wb.b, cfa_pattern, ccm, c, g, output_img);
        
        // transpose to account for interleaved layout
        output_img.transpose(0, 1);
        output_img.transpose(1, 2);
        
        return output_img;
    }
    
    static bool save_png(const std::string& dir_path, const std::string& img_name, const Halide::Runtime::Buffer<uint8_t> &img) {
        const std::string img_path = dir_path + "/" + img_name;

        const int stride_in_bytes = img.width() * img.channels();
        if (!stbi_write_png(img_path.c_str(), img.width(), img.height(), img.channels(), img.data(), stride_in_bytes)) {
            std::cerr << "Unable to write output image '" << img_name << "'" << std::endl;
            return false;
        }
        return true;
    }
};


int main(int argc, char* argv[]) {
    
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " [-c comp -g gain (optional)] dir_path out_img raw_img1 raw_img2 [...]" << std::endl;
        return 1;
    }

    Compression c = 3.8f;
    Gain g = 1.1f;

    int i = 1;

    while(argv[i][0] == '-') {
        if(argv[i][1] == 'c') {
            c = atof(argv[++i]);
            i++;
            continue;
        } else if(argv[i][1] == 'g') {
            g = atof(argv[++i]);
            i++;
            continue;
        } else {
            std::cerr << "Invalid flag '" << argv[i][1] << "'" << std::endl;
            return 1;
        }
    }

    if (argc - i < 4) {
        std::cerr << "Usage: " << argv[0] << " [-c comp -g gain (optional)] dir_path out_img raw_img1 raw_img2 [...]" << std::endl;
        return 1;
    }

    std::string dir_path = argv[i++];
    std::string out_name = argv[i++];

    std::vector<std::string> in_names;
    while (i < argc) {
        in_names.emplace_back(argv[i++]);
    }

    Burst burst(dir_path, in_names);

    HDRPlus hdr_plus(burst, c, g);

    Halide::Runtime::Buffer<uint8_t> output = hdr_plus.process();

    if (!HDRPlus::save_png(dir_path, out_name, output)) {
        return EXIT_FAILURE;
    }

    return 0;
}
