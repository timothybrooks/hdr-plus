# HDR+ Implementation
Original Document on the subject (by Timothy Brooks): https://timothybrooks.com/tech/hdr-plus

Currently maintained by [Simon Gardling](https://github.com/Titaniumtown)

### Quick Start (Nix Flakes)

Build the package directly:
```bash
nix build
./result/bin/hdrplus
```

Or enter a development shell:
```bash
nix develop
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### HDR+ algorithm examples:
You can download the examples from [this torrent](./hdr_plus_examples.torrent). Make sure to seed!

### Compiled Binary Usage:
```
Usage: ./hdrplus [-c comp -g gain (optional)] dir_path out_img raw_img1 raw_img2 [...]
```

The -c and -g flags change the amount of dynamic range compression and gain respectively. Although they are optional because they both have default values. 
