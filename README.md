Note: I now work for Google's computational photography research group, which developed and published HDR+. Therefore, to avoid conflicts of interest, I no longer maintain the code in this repository. If you are interested in continuing this open source project, I would love to transfer ownership to another open source contributor. Please reach out to me if you are interested!

# HDR+ Implementation
Please see: http://timothybrooks.com/tech/hdr-plus

To compile, follow these steps:
1. Install libraw¹, libpng, and libjpeg.²
2. Download a version of Halide from https://github.com/halide/Halide_old_history/releases.
3. Set `HALIDE_ROOT_DIR` in CMakeLists.txt to the Halide directory path.
4. From the project root directory, run the following commands:
```
mkdir build
cd build
cmake ..
make
```

Usage:
```
./hdrplus [options] dir_path out_img raw_img1 raw_img2 [...]
```
Examples:
```
./hdrplus ~/Desktop output.png 1.dng 2.dng 3.dng
```

Footnotes:

¹ If you are on macOS the included dcraw binary will not work so I included the macos version in the same directory. If you are on macOS change the following "../tools/dcraw" to "../tools/dcraw_macos" in the files below before compiling:
  - Batch_dcraw.py
  - finish.cpp (2 instances)
  - HDRPlus.cpp
  - halide_load_raw.h
  
² Also to install libraw, libpng, and libjpeg on macOS run ```brew install libraw* libpng* libjpeg*```
