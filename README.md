Note: I now work for Google's computational photography research group, which developed and published HDR+. Therefore, to avoid conflicts of interest, I no longer maintain the code in this repository. If you are interested in continuing this open source project, I would love to transfer ownership to another open source contributor. Please reach out to me if you are interested!

# HDR+ Implementation
Please see: http://timothybrooks.com/tech/hdr-plus

To compile, follow these steps:
1. Install Halide from http://halide-lang.org/.
2. Set `HALIDE_ROOT_DIR` in CMakeLists.txt to the Halide directory path.
3. From the project root directory, run the following commands:
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
