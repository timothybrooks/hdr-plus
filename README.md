Note: The user "Titaniumtown" is now the primary maintainer of this repository; if you need to contact the owner contact "Titaniumtown".

# HDR+ Implementation
Original Document on the subject (by Timothy Brooks): http://timothybrooks.com/tech/hdr-plus

### Compilation instructions:
1. Install libraw, libpng, and libjpeg.¹
2. Download and compile llvm 3.9
3. Download the "2017-05-03" source of Halide from https://github.com/halide/Halide_old_history/releases
4. Compile Halide according to the README.md file included with the source file zip of halide.
5. Go the folder you have the hdr-plus code in.
6. Set `HALIDE_ROOT_DIR` in CMakeLists.txt to the Halide directory path.
7. From the project root directory, run the following commands:
```
mkdir build
cd build
cmake ..
make
```

### HDR+ algorithm examples:

Timothy Brooks provided images (at the link below) that have burst shot inputs and the HDR+'s output.

https://drive.google.com/drive/folders/1XR61IhzuYQU5eLQfBJ0KEMK83Aocta6g?usp=sharing

### Compiled Binary Usage:
```
Usage: ./hdrplus [-c comp -g gain (optional)] dir_path out_img raw_img1 raw_img2 [...]
```

The -c and -g flags change the amount of dynamic range compression and gain respectively. Although they are optional because they both have default values. 

### Footnotes:
  
¹Also to install libraw, libpng, and libjpeg on macOS run ```brew install libraw libpng libjpeg```
