Note: The user "Titaniumtown" is now the primary maintainer of this repository; if you need to contact the owner contact "Titaniumtown".

# HDR+ Implementation
Original Document on the subject (by Timothy Brooks): http://timothybrooks.com/tech/hdr-plus

### Compilation instructions:
1. Install libraw, libpng, and libjpeg.¹
2. Download and compile llvm 3.9
3. Install or compile the latest version Halide
4. Go the folder you have the hdr-plus code in.
5. From the project root directory, run the following commands:
```
mkdir build
cd build
cmake -DHALIDE_DISTRIB_DIR=$(Insert halide directory) ..
make -j$(expr $(nproc) \+ 1)
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
