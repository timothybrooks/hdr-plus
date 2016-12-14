#include "Halide.h"
#include "halide_load_raw.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

#include "align.h"
#include "merge.h"
#include "finish.h"

using namespace Halide;

// TODO: error handling should be more robust
// To check invariants we currently use assertions.

/*
 * HDRPlus Class -- houses file I/O and calls stages of the main pipeline.
 * Also defines attributes specific to our implementation such as the static
 * cropped image dimensions
 */
class HDRPlus {

    private:

        const Image<uint16_t> imgs;

    public:

        // dimensions of pixel phone output images are 3036 x 4048
        // rounded down so both dimensions are a multiple of 16
        // which helps avoid corner cases in align and merge
        static const int width = 4032;
        static const int height = 3024;

        const BlackPoint bp;
        const WhitePoint wp;
        const WhiteBalance wb;
        const Compression c;
        const Gain g;

        // The reference image will always be the first image
        HDRPlus(Image<uint16_t> imgs, BlackPoint bp, WhitePoint wp, WhiteBalance wb, Compression c, Gain g) : imgs(imgs), bp(bp), wp(wp), wb(wb), c(c), g(g) {

            assert(imgs.dimensions() == 3);         // width * height * img_idx
            assert(imgs.width() == width);
            assert(imgs.height() == height);
            assert(imgs.extent(2) >= 2);            // must have at least one alternate image
        }

        /*
         * process -- calls all of the main stages of the pipeline aside from file I/O
         */
        Image<uint8_t> process() {

            // These three steps of the pipeline will be defined in the other cpp files for our own organization.
            // In the future we can decide if it is better to include them in this class

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
         * load_raws -- load in a vector of CR2 (Canon Raw) files and verify that they all have the same dimensions
         */
        static bool load_raws(std::string dir_path, std::vector<std::string> &img_names, Image<uint16_t> &imgs) {

            int num_imgs = img_names.size();

            imgs = Image<uint16_t>(width, height, num_imgs);

            for (int n = 0; n < num_imgs; n++) {

                std::string img_name = img_names[n];
                std::string img_path = dir_path + "/" + img_name;

                Image<uint16_t> img;

                if(!Tools::load_raw(img_path, &img)) {
                    std::cerr << "Input image failed to load" << std::endl;
                    return false;
                }

                int img_width  = img.width();
                int img_height = img.height();
                
                if (img.dimensions() != 2) {
                    std::cerr << "Input image '" << img_name << "' has " << img.dimensions() << " dimensions, but must have exactly 2" << std::endl;
                    return false;
                }
                if (img_width < width) {
                    std::cerr << "Input image '" << img_name << "' has width " << img_width << ", but must must have width >= " << width << std::endl;
                    return false;
                }
                if (img_height < height) {
                    std::cerr << "Input image '" << img_name << "' has height " << img_height << ", but must have height >= " << height << std::endl;
                    return false;
                }

                // offsets must be multiple of two to keep bayer pattern
                // We crop the image here to simplify the rest of the pipeline.
                // e.g. It's useful to have an image that is a multiple of half the tile size.
                int x_offset = ((img_width  - width ) / 4) * 2;
                int y_offset = ((img_height - height) / 4) * 2; 

                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {

                        imgs(x, y, n) = img(x_offset + x, y_offset + y);
                    }
                }
            }
            return true;
        }

        /*
         * save_png -- writes processed image to specified img_name
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

        /*
         * find_ref -- currently unused. Finds the frame in a stack with the smallest variance.
         * This frame would ideally be used as the reference frame for the rest of the pipeline.
         * Based on Welford's numerically stable online variance approximation.
         */

        static int find_ref(Halide::Image<uint16_t> &imgs, int min_idx = 0, int max_idx = 3) {

            assert(imgs.dimensions() == 3);

            Func variance("variance");
            Var n;
            variance(n) = 0; // TODO: compute variance within some smaller region of image n

            double max_variance = 0;
            int ref_idx = 0;

            Image<double> variance_r = variance.realize(imgs.extent(0));

            for (int idx = min_idx; idx < max_idx; idx++) {

                double curr_variance = variance_r(idx);
                if (curr_variance > max_variance) {

                    max_variance = curr_variance;
                    ref_idx = idx;
                }
            }

            return ref_idx;
        }
};

int main(int argc, char* argv[]) {
    
    // TODO: validate input arguments
    
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " dir_path out_img raw_img1 raw_img2 [...]" << std::endl;
        return 1;
    }

    int i = 1;
    std::string dir_path = argv[i++];
    std::string out_name = argv[i++];

    std::vector<std::string> in_names;

    while (i < argc) in_names.push_back(argv[i++]);

    Image<uint16_t> imgs;

    if(!HDRPlus::load_raws(dir_path, in_names, imgs)) return -1;

    // TODO: find reference, reorder so reference is first image
    // This is not super important to figure out, since it is easy
    // and for now we just use the first image
    //int ref_idx = HDRPlus::find_ref(imgs);

    // TODO: read these values from the reference image header (possibly using dcraw)
    const WhiteBalance wb = {1.3984, 1, 1, 2.415};   // r, g0, g1, b
    const BlackPoint bp = 2050;
    const WhitePoint wp = 15464;
    const Compression c = 3.5f;
    const Gain g = 1.2f;

    HDRPlus hdr_plus = HDRPlus(imgs, bp, wp, wb, c, g);

    // This image has an RGB interleaved memory layout
    Image<uint8_t> output = hdr_plus.process();
    
    if(!HDRPlus::save_png(dir_path, out_name, output)) return -1;

    return 0;
}