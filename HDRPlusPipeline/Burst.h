#include<iostream>
#include <vector>

#include "Halide.h"
#include "RAWImage.h"
#include "Image.h"

#define MAX_BURST_LENGTH 10

using namespace Halide;

/*
 * Burst -- 
 * 
 * Class containing methods relating to the align and merge stages of the pipeline. The Burst selects
 * a reference frame to base its results on, and attempts to align patches of the other images with that
 * reference frame. If reasonable matches are found, the matching patch is blended into the reference
 * frame in the merge step.
 */

class Burst {
    private:
        std::string name;
        size_t burstLength;
        size_t width;
        size_t height;
        RAWImage* reference;
        std::vector<RAWImage*> alternates;

        void align(void) {
            return;
        }

    public:
        //Constructor
        Burst(std::string name, std::vector<std::string> imFileNames);

        //Compute the merged raw image by aligning patches of the alternates with the reference frame.
        RawImage merge(void);

        //Destructor
        virtual ~Burst() {
            for (size_t i = 0; i < this->burstLength-1; i++) {
                delete this->alternates[i];
            }
            delete this->reference;
        };


};