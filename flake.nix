{
  description = "HDR+ Implementation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        hdr-plus = pkgs.stdenv.mkDerivation {
          pname = "hdr-plus";
          version = "0.1.0";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            gnumake
            pkg-config
          ];

          buildInputs = with pkgs; [
            halide
            libtiff
            libpng
            libjpeg
            zlib
            libraw
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
          ];

          # The build produces two binaries: hdrplus and stack_frames
          installPhase = ''
            runHook preInstall

            mkdir -p $out/bin
            cp hdrplus $out/bin/
            cp stack_frames $out/bin/

            runHook postInstall
          '';

          meta = with pkgs.lib; {
            description = "HDR+ burst photography implementation";
            homepage = "https://github.com/timothybrooks/hdr-plus";
            license = licenses.mit;
            platforms = platforms.unix;
            mainProgram = "hdrplus";
          };
        };
      in
      {
        packages = {
          default = hdr-plus;
          hdr-plus = hdr-plus;
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ hdr-plus ];

          packages = with pkgs; [
            clang-tools
          ];
        };
      }
    );
}
