#include <cstdlib>
#include <vector>

//#include "Image.h"
//#include "RAWImage.h"
#include "Burst.h"

int main(int argc, char* argv[]) {
	//Image i = Image("../images/example.png");
	//i.write("../images/out.png");
    std::vector<std::string> imFileNames = {"../testImages/1_blur.png", "../testImages/1_sharp.png"};
    Burst("testBurst", imFileNames);
	return 0;
}