#include <stdlib.h>
#include "Image.h"

int main(int argc, char* argv[]) {
	Image i = Image("../images/example.bmp");
	i.write("../images/out.png");

	return 0;
}