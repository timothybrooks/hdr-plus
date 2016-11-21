#include "Burst.h"

class Burst {
    private:

        void findReferenceFrame(void) {
            return;
        }

        void align(void) {
            return;
        }

    public:

        Burst(std::string name, std::vector<std::string> imFileNames) : name(name), burstLength(imFileNames.size()) {
            if (this->burstLength < 2) {
                throw std::exception("too few images in burst");
            } else if (this->burstLength > MAX_BURST_LENGTH) {
                std::cout << "Too many images in burst. Ignoring beyond the first " << MAX_BURST_LENGTH << " frames." << std::endl;
                this->burstLength = MAX_BURST_LENGTH;
            }
            this->alternates.resize(this->burstLength-1);
            this->reference = new RAWImage(imFileNames[0]);
            size_t bestVariance = this->reference->getVariance(); //TODO IMPLEMENT GETVARIANCE FOR RAWIMAGES
            RAWImage* currRAWImage = NULL;
            size_t currVariance;
            
            //TODO profile this to see whether it could be improved with parallelism. Probably bandwidth-bound
            for (size_t i = 1; i < std::min(this->burstLength,3); i++) {
                currRAWImage = new RAWImage(imFileNames[i]);
                currVariance = currRAWImage->getVariance();
                if (currVariance > bestVariance) {
                    this->alternates[i-1] = this->reference;
                    bestVariance = currVariance;
                    this->reference = currRAWImage;
                } else {
                    this->alternates[i-1] = currRAWImage;
                }
            }
            //only consider first three frames as potential reference
            for (size_t i = 3; i < this->burstLength, i++) {
                this->alternates[i-1] = new RAWImage(imFileNames[i]);
            }
            this->width = this->reference->getWidth();
            this->height = this->reference->getHeight();
            this->metadata = this->reference->getMetadata(); //TODO IMPLEMENT GETMETADATA FOR RAWIMAGES
        }

        RAWImage merge(void) {
            return RAWImage();
        }

        //Destructor
        virtual ~Burst() {};
}