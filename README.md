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

Height and Width related errors:

If you get an error relating to the width and height of the input photos change the width and height in src/HDRPlus.cpp to the dimensions of your input files.

  Example of an error created:
  ```
  Input image '/Users/simon/Desktop/Photography/HDR-PLUS-TEST/1//3.dng' has width 3474, but must must have width of 5796
  ```

Usage:
```
./hdrplus [options] dir_path out_img raw_img1 raw_img2 [...]
```

Footnotes:

¹ If you are on macOS the included dcraw binary will not work so included is the macos version in the same directory. If you are on macOS change the following "../tools/dcraw" to "../tools/dcraw_macos" in the files below before compiling:
  - batch_dcraw.py
  - finish.cpp (2 instances)
  - HDRPlus.cpp
  - halide_load_raw.h
  
² Also to install libraw, libpng, and libjpeg on macOS run ```brew install libraw* libpng* libjpeg*```
