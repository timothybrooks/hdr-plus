#include<iostream>
#include <vector>

#include "Halide.h"
#include "RAWImage.h"
#include "Image.h"

#define MAX_BURST_LENGTH 10

using namespace Halide;

/*
 * Image -- 
 * 
 * Class containing methods relating to the output data for
 * the HDRPlus pipeline. This includes toneMapping and other
 * methods for finishing based on the metadata passed up by
 * the merge and align steps as well as the RAW reference
 * image's metadata.
 */

class Burst {
    private:
        std::string name;
        size_t width;
        size_t height;
        RAWImage* reference;
        std::vector<RAWImage*> alternates;

        //finds the best frame in the burst to be used as the reference
        //for the rest of the pipeline. Removes that frame from the
        //alternates. Currently the criteria for "best" ref frame are based
        //on the HDR+ Burst paper: The ref frame is the sharpest of the first
        //three RAWImages in the burst. Also stores reference frame metadata.
        void findReferenceFrame(void);

    public:
        //Constructor
        Burst(std::string name, std::vector<std::string> imFileNames);

        //Compute the merged raw image by aligning patches of the alternates with the reference frame.
        RawImage merge(void);

        //Destructor
        virtual ~Burst() {};


};