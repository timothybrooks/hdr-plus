with import <nixpkgs> {};

stdenv.mkDerivation {
    name = "hdr-plus";
    buildInputs = [
        cmake
        halide
        libtiff
        libpng
    ];
}
