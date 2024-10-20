# HDR+ Implementation
Original Document on the subject (by Timothy Brooks): https://timothybrooks.com/tech/hdr-plus

Currently maintained by [Simon Gardling](https://github.com/Titaniumtown)

### Compilation instructions:
1. Install the [Nix](https://nixos.org/) package manager
2. Run the below commands:
```
nix-shell -I shell.nix # enter a development environment
mkdir build
cd build
cmake ..
make -j`nproc`
```

### HDR+ algorithm examples:
There are three zip files with examples of the HDR+ algorithm at: https://www.gardling.com/hdr_plus

### Compiled Binary Usage:
```
Usage: ./hdrplus [-c comp -g gain (optional)] dir_path out_img raw_img1 raw_img2 [...]
```

The -c and -g flags change the amount of dynamic range compression and gain respectively. Although they are optional because they both have default values. 
