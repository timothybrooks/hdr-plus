#ifndef HDRPLUS_MERGE_H_
#define HDRPLUS_MERGE_H_

#include "Halide.h"

Func merge(Image imgs, Func alignment) {

	// TODO: figure out what we need to do lol
	// Weiner filtering or something like that...

	Var x, y;
	Func output(imgs->extent(0), imgs->extent(1));
	output(x, y) = imgs(x, y, 0); // for now, just set output to reference image

	return output;
}

#endif