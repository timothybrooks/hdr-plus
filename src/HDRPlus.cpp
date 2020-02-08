#include "Halide.h"
#include "halide_load_raw.h"

#include "Burst.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"
#include <stdio.h>
#include "align.h"
#include "merge.h"
#include "finish.h"
#include <iostream>
#include <fstream>

using namespace Halide;
using namespace std;
/*
 * HDRPlus Class -- Houses file I/O, defines pipeline attributes and calls
 * processes main stages of the pipeline.
 */
class HDRPlus {

private:

    const Halide::Runtime::Buffer<uint16_t> imgs;

public:


    const int width;
    const int height;

    const BlackPoint bp;
    const WhitePoint wp;
    const WhiteBalance wb;
    const Compression c;
    const Gain g;
    char *readfile;
    std::string readfilestring;
    void imagesize();

    HDRPlus(Halide::Runtime::Buffer<uint16_t> imgs, BlackPoint bp, WhitePoint wp, WhiteBalance wb, Compression c, Gain g)
        : imgs(imgs)
        , width(imgs.width())
        , height(imgs.height())
        , bp(bp)
        , wp(wp)
        , wb(wb)
        , c(c)
        , g(g)
    {
        assert(imgs.dimensions() == 3);         // width * height * img_idx
        assert(imgs.extent(2) >= 2);            // must have at least one alternate image
    }

    /*
     * process -- Calls all of the main stages (align, merge, finish) of the pipeline.
     */
    Buffer<uint8_t> process() {
        Halide::Buffer<uint16_t> imgsBuffer(*imgs.raw_buffer());

        Func alignment = align(imgsBuffer);
        Func merged = merge(imgsBuffer, alignment);
        Func finished = finish(merged, width, height, bp, wp, wb, c, g);

        ///////////////////////////////////////////////////////////////////////////
        // realize image
        ///////////////////////////////////////////////////////////////////////////

        Buffer<uint8_t> output_img(3, width, height);
        finished.compile_jit();
        finished.realize(output_img);

        // transpose to account for interleaved layout

        output_img.transpose(0, 1);
        output_img.transpose(1, 2);

        return output_img;
    }

    /*
     * save_png -- Writes an interleaved Halide image to an output file.
     */
    static bool save_png(std::string dir_path, std::string img_name, Buffer<uint8_t> &img) {

        std::string img_path = dir_path + "/" + img_name;

        std::remove(img_path.c_str());

        int stride_in_bytes = img.width() * img.channels();

        if(!stbi_write_png(img_path.c_str(), img.width(), img.height(), img.channels(), img.data(), stride_in_bytes)) {

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
        }
        else if(argv[i][1] == 'g') {

            g = atof(argv[++i]);
            i++;
            continue;
        }
        else {
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
    while (i < argc) in_names.push_back(argv[i++]);

    Burst burst(dir_path, in_names);
    Halide::Runtime::Buffer<uint16_t> imgs = burst.ToBuffer();
    if (imgs.channels() < 2) {
        return EXIT_FAILURE;
    }

    HDRPlus hdr_plus(
        imgs,
        burst.GetBlackLevel(),
        burst.GetWhiteLevel(),
        burst.GetWhiteBalance(),
        c,
        g);

    Buffer<uint8_t> output = hdr_plus.process();

    if(!HDRPlus::save_png(dir_path, out_name, output)) return -1;

    return 0;
}
