{
  description = "Light Field Image Format development and build environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    xvc-src = {
      url = "github:divideon/xvc";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, xvc-src }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
      pkgsFor = system: import nixpkgs {
        inherit system;
        overlays = [ self.overlays.default ];
      };
    in {
      overlays.default = final: prev: {
        xvc = final.stdenv.mkDerivation {
          pname = "xvc";
          version = "2.0-unstable-2025-07-03";
          src = xvc-src;

          nativeBuildInputs = [ final.cmake final.ninja final.pkg-config ];
          cmakeFlags = [
            "-DBUILD_APPS=OFF"
            "-DBUILD_SHARED_LIBS=ON"
            "-DBUILD_TESTS=OFF"
            "-DBUILD_TESTS_LIBS=OFF"
            "-DCMAKE_INSTALL_LIBDIR=lib"
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
          ];

          postPatch = ''
            substituteInPlace cmake/xvc.pc.in \
              --replace-fail "Version: 1.0" \
                             "Version: 2.0-unstable-2025-07-03"
          '';

          meta = {
            description = "xvc encoder and decoder libraries";
            homepage = "https://github.com/divideon/xvc";
            license = final.lib.licenses.lgpl21Plus;
            platforms = final.lib.platforms.linux;
          };
        };
      };

      packages = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in {
          xvc = pkgs.xvc;
          default = pkgs.stdenv.mkDerivation {
            pname = "lfif";
            version = "2.0.0";
            src = self;

            nativeBuildInputs = [ pkgs.cmake pkgs.ninja pkgs.pkg-config ];
            cmakeFlags = [
              "-DBUILD_TESTING=OFF"
              "-DLFIF_BUILD_EXTRAS=OFF"
            ];
          };
        });

      checks = forAllSystems (system: {
        inherit (self.packages.${system}) default xvc;
      });

      devShells = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in {
          default = pkgs.mkShell {
            packages = [
              pkgs.cmake
              pkgs.ninja
              pkgs.pkg-config
              pkgs.gdb
              pkgs.clang-tools
              pkgs.doxygen
              pkgs.gtest
              pkgs.ffmpeg
              pkgs.ffmpeg.dev
              pkgs.openjpeg
              pkgs.mozjpeg
              pkgs.xvc
            ];

            MOZJPEG_ROOT = "${pkgs.mozjpeg}";
          };
        });
    };
}
