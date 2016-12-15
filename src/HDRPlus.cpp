#include "Halide.h"
#include "halide_load_raw.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

#include "align.h"
#include "merge.h"
#include "finish.h"

using namespace Halide;

/*
 * HDRPlus Class -- Houses file I/O, defines pipeline attributes and calls
 * processes main stages of the pipeline.
 */
class HDRPlus {

    private:

        const Image<uint16_t> imgs;

    public:

        // dimensions of pixel phone output images are 3036 x 4048

        static const int width = 5796;
        static const int height = 3870;

        const BlackPoint bp;
        const WhitePoint wp;
        const WhiteBalance wb;
        const Compression c;
        const Gain g;

        HDRPlus(Image<uint16_t> imgs, BlackPoint bp, WhitePoint wp, WhiteBalance wb, Compression c, Gain g) : imgs(imgs), bp(bp), wp(wp), wb(wb), c(c), g(g) {

            assert(imgs.dimensions() == 3);         // width * height * img_idx
            assert(imgs.width() == width);
            assert(imgs.height() == height);
            assert(imgs.extent(2) >= 2);            // must have at least one alternate image
        }

        /*
         * process -- Calls all of the main stages (align, merge, finish) of the pipeline.
         */
        Image<uint8_t> process() {

            Func alignment = align(imgs);
            Func merged = merge(imgs, alignment);
            Func finished = finish(merged, width, height, bp, wp, wb, c, g);

            ///////////////////////////////////////////////////////////////////////////
            // realize image
            ///////////////////////////////////////////////////////////////////////////

            Image<uint8_t> output_img(3, width, height);

            finished.realize(output_img);

            // transpose to account for interleaved layout

            output_img.transpose(0, 1);
            output_img.transpose(1, 2);

            return output_img;
        }

        /*
         * load_raws -- Loads CR2 (Canon Raw) files into a Halide Image.
         */
        static bool load_raws(std::string dir_path, std::vector<std::string> &img_names, Image<uint16_t> &imgs) {

            int num_imgs = img_names.size();

            imgs = Image<uint16_t>(width, height, num_imgs);

            uint16_t *data = imgs.data();

            for (int n = 0; n < num_imgs; n++) {

                std::string img_name = img_names[n];
                std::string img_path = dir_path + "/" + img_name;

                if(!Tools::load_raw(img_path, data, width, height)) {

                    std::cerr << "Input image failed to load" << std::endl;
                    return false;
                }

                data += width * height;
            }
            return true;
        }

        /*
         * save_png -- Writes an interleaved Halide image to an output file.
         */
        static bool save_png(std::string dir_path, std::string img_name, Image<uint8_t> &img) {

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

/*
 * read_white_balance -- Reads white balance multipliers from file and returns WhiteBalance.
 */
const WhiteBalance read_white_balance(std::string file_path) {

    Tools::Internal::PipeOpener f(("../tools/dcraw -v -i " + file_path).c_str(), "r");
    
    char buf[1024];

    while(f.f != nullptr) {

        f.readLine(buf, 1024);

        float r, g0, g1, b;

        if(sscanf(buf, "Camera multipliers: %f %f %f %f", &r, &g0, &b, &g1) == 4) {

            float m = std::min(std::min(r, g0), std::min(g1, b));

            return {r / m, g0 / m, g1 / m, b / m};
        }
    }

    return {1, 1, 1, 1};
}

int main(int argc, char* argv[]) {
    
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " [options] dir_path out_img raw_img1 raw_img2 [...]" << std::endl;
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
        std::cerr << "Usage: " << argv[0] << " [-c comp -g gain] dir_path out_img raw_img1 raw_img2 [...]" << std::endl;
        return 1;
    }

    std::string dir_path = argv[i++];
    std::string out_name = argv[i++];

    std::vector<std::string> in_names;

    while (i < argc) in_names.push_back(argv[i++]);

    Image<uint16_t> imgs;

    if(!HDRPlus::load_raws(dir_path, in_names, imgs)) return -1;

    const WhiteBalance wb = read_white_balance(dir_path + "/" + in_names[0]);
    const BlackPoint bp = 2050;
    const WhitePoint wp = 15464;

    HDRPlus hdr_plus = HDRPlus(imgs, bp, wp, wb, c, g);

    Image<uint8_t> output = hdr_plus.process();
    
    if(!HDRPlus::save_png(dir_path, out_name, output)) return -1;

    return 0;
}