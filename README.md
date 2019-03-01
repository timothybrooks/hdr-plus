Note: The user "Titaniumtown" is now the primary maintainer of this repository; if you need to contact the owner contact "Titaniumtown".

# HDR+ Implementation
Original Document on the subject (by Timothy Brooks): http://timothybrooks.com/tech/hdr-plus

### Compilation instructions:
1. Install libraw¹, libpng, and libjpeg.²
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

### Height and Width related errors:

If you get an error relating to the width and height of the input photos³ change the width and height in src/HDRPlus.cpp to the dimensions of your input files.

#### Example of an error created:
```
Input image '/Users/simon/Desktop/Photography/HDR-PLUS-TEST/1//3.dng' has width 3474, but must must have width of 5796
```

### Wp and Bp issues

If you get a discolored or weird looking image, do the command ``` dcraw -v -T (filename)``` and make note of the "darkness" and "saturation" numbers. Delete the tiff file dcraw made as well. Open HDRPlus.cpp and find "const BlackPoint bp =" and replace the number following that with the "darkness" number from the dcraw command. The line below that contains "const WhitePoint wp =" replace the number after this with the "saturation" number you got from dcraw. Recompile and it it should work! If you get the same issue with other pictures do the same thing again, but run the dcraw command on the other picture.

#### Example output of Dcraw command:
(Note: The important values are on line 2 of the output)
```
Loading Canon EOS DIGITAL REBEL XT image from 1.CR2 ...
Scaling with darkness 255, saturation 4095, and
multipliers 2.690726 1.000000 1.270038 1.000000
AHD interpolation...
Converting to sRGB colorspace...
Writing data to 1.tiff ...
```


### Compiled Binary Usage:
```
Usage: ./hdrplus [-c comp -g gain (optional)] dir_path out_img raw_img1 raw_img2 [...]
```

The -c and -g flags change the amount of dynamic range compression and gain respectively. Although they are optional because they both have default values. 


### Footnotes:

¹ If you are on macOS the included dcraw binary will not work so I included the macos version in the same directory. If you are on macOS change the following "../tools/dcraw" to "../tools/dcraw_macos" in the files below before compiling:
  - batch_dcraw.py
  - finish.cpp (2 instances)
  - HDRPlus.cpp
  - halide_load_raw.h
  
² Also to install libraw, libpng, and libjpeg on macOS run ```brew install libraw* libpng* libjpeg*```

³ I am creating a prototype of the HDR+ code that uses Exiv2 to detect black and white levels along with width and height; so stay tuned!
