#include "Halide.h"
#include "align.h"
#include "merge.h"
#include "finish.h"

// It is ok for me to use the Halide namespace within the c++ file. It just shouldn't be used in a header file!
using namespace Halide;

// TODO: error handling should be more robust
// to check invariants, I use assertions
// when something fails from main, I return -1 without printing to stderr

// I think this is ok for now and we should wait till later to clean things up
// More importantly, we should implement the Align, Merge and Finsh functions

class HDRPlus {

    private:

        const Image imgs;

    public:

        const int width = 1920;
        const int height = 1080;

        // The reference image will always be the first image
        HDRPlus(Image<uint16_t> imgs) : imgs(imgs) {

            assert(imgs.dimmensions() == 3);        // width * height * img_idx
            assert(imgs.extent(0) == width);
            assert(imgs.extent(1) == height);
            assert(imgs.extent(2) >= 2);            // must have at least one alternate image
        }

        Image process() {

            // These three steps of the pipeline will be defined in the other cpp files for our own organization.
            // In the future we can decide if it is better to include them in this class
            Func alignment = align(imgs);
            Image output = merge(imgs, alignmnet);
            return finish(output);
        }

};

bool load_grayscale_imgs(std::vector<std::string> &img_names, std::string img_dir, Image<uint8_t> &imgs) {

    assert(imgs.dimensions() == 3);

    int width = imgs.extent(0);
    int height = imgs.extent(1);
    int num_imgs = img_names.size();

    for (int n = 0; n < num_imgs; n++) {

        std::string img_path = img_dir + "/" + img_names[i];

        Image<uint8_t> img;
        if (!Tools::load_image(img_path, img)) {
            return false;
        }

        assert(img.dimmensions() = 2);
        assert(img.extent(0) == width);
        assert(img.extent(1) == height);

        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                imgs(x, y, n) = img(x, y);
            }
        }
    }
    return true;
}

int find_reference(Halide::Image<uint8_t> &imgs, int min_idx = 0; int max_idx = 3) {
    assert(imgs->dimensions() == 3);

    Func variance("variance");
    Var n;
    variance(n) = 0; // TODO: compute variance within some smaller region of image n

    max_variance = 0;
    ref_idx = 0;

    for (int idx = min_idx; idx < max_idx idx++) {

        double curr_variance = variance(idx);
        if (curr_variance > max_variance) {

            max_variance = curr_variance;
            ref_idx = idx;
        }
    }

    return ref_idx;
}

int main(int argc, char* argv[]) {
    
    // TODO: get from commend line arguments
    std::vector<std::string> img_names = {"input-1.png", "input-2.png"};
    std::string img_dir = "../images"
    std::string output_name = "output.png"

    Image<uint8_t> imgs(HDRPlus::width, HDRPlus::height, img_names.size());

    if(!load_grayscale_imgs(img_names, img_dir, imgs)) {
        return -1;
    }

    // TODO: find reference, reorder so reference is first image
    // This is not super important to figure out, since it is easy
    // and for now we just use the first image
    //int ref_idx = find_reference(imgs);

    HDRPlus hdr_plus = HDRPlus(imgs);
    Image output = hdr_plus.process();
    
    if(!Tools::save(output, img_dir + "/" + output_name)) {
        return -1;
    }

    return 0;
}