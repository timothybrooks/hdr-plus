#ifndef HDRPLUS_MERGE_H_
#define HDRPLUS_MERGE_H_

#include "Halide.h"

using namespace Halide;

Image<uint8_t> merge(Image<uint8_t> imgs, Func alignment) {

	// TODO: figure out what we need to do lol
	// Weiner filtering or something like that...

	Var x, y;
	Image<uint8_t> output(imgs.extent(0), imgs.extent(1));
	output(x, y) = imgs(x, y, 0); // for now, just set output to reference image

	return output;
}

#endif