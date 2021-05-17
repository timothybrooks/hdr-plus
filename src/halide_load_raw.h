// basic raw image loader for Halide::Buffer<T> or other image with same API
// code largely appropriated from halide_image_io.h

#ifndef HALIDE_LOAD_RAW_H
#define HALIDE_LOAD_RAW_H

#include "Halide.h"
#include <libraw/libraw.h>

namespace Halide {
namespace Tools {

namespace Internal {

struct PipeOpener {
    PipeOpener(const char* cmd, const char* mode) : f(popen(cmd, mode)) {
        // nothing
    }
    ~PipeOpener() {
        if (f != nullptr) {
            pclose(f);
        }
    }
    // read a line of data skipping lines that begin with '#"
    char *readLine(char *buf, int maxlen) {
        char *status;
        do {
            status = fgets(buf, maxlen, f);
        } while(status && buf[0] == '#');
        return(status);
    }
    FILE * const f;
};

} // namespace Internal

inline bool is_little_endian() {
    int value = 1;
    return ((char *) &value)[0] == 1;
}

inline void swap_endian_16(uint16_t &value) {
    value = ((value & 0xff)<<8)|((value & 0xff00)>>8);
}

template<Internal::CheckFunc check = Internal::CheckFail>
bool load_raw(const std::string &filename, uint16_t* data, int width, int height) {
    LibRaw RawProcessor;
    int ret = RawProcessor.dcraw_process();
    
    if (LIBRAW_SUCCESS != ret)
    {
        fprintf(stderr, "Cannot open %s: %s\n", filename, libraw_strerror(ret));
        return ret;
    }
    
    return true;
}

} // namespace Tools
} // namespace Halide

#endif
