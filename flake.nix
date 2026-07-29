{
  description = "Light Field Image Format";

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
      pkgsFor = system: import nixpkgs { inherit system; };
      xvcFor = system:
        let
          pkgs = pkgsFor system;
        in
        pkgs.gcc16Stdenv.mkDerivation {
          pname = "xvc";
          version = "2.0-unstable-2025-07-03";
          src = xvc-src;

          nativeBuildInputs = [ pkgs.cmake pkgs.ninja pkgs.pkg-config ];
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
        };
    in {
      packages = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in {
          xvc = xvcFor system;
          default = pkgs.gcc16Stdenv.mkDerivation {
            pname = "lfif";
            version = "2.0.0";
            src = self;

            nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
            cmakeFlags = [
              "-DBUILD_TESTING=OFF"
              "-DLFIF_BUILD_EXTRAS=OFF"
            ];

            installPhase = ''
              mkdir -p $out/bin $out/include $out/lib
              cp tools/lfif $out/bin/
              cp liblfif/liblfif.a lfif_container/liblfif_container.a libppm/libppm.a $out/lib/
              cp -R ../liblfif/include/. ../lfif_container/include/. ../libppm/include/. $out/include/
            '';
          };
        });

      checks = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in {
          full = pkgs.gcc16Stdenv.mkDerivation {
            pname = "lfif-check";
            version = "2.0.0";
            src = self;

            nativeBuildInputs = [ pkgs.cmake pkgs.ninja pkgs.pkg-config ];
            buildInputs = [
              pkgs.gtest
              pkgs.ffmpeg
              pkgs.openjpeg
              pkgs.mozjpeg
              (xvcFor system)
            ];
            MOZJPEG_ROOT = "${pkgs.mozjpeg}";

            configurePhase = "cmake --preset full-check";
            buildPhase = "cmake --build --preset full-check";
            checkPhase = "ctest --preset full-check";
            doCheck = true;
            installPhase = "mkdir -p $out";
          };
        });

      devShells = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in {
          default = pkgs.mkShell.override {
            stdenv = pkgs.gcc16Stdenv;
          } {
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
              (xvcFor system)
            ];

            MOZJPEG_ROOT = "${pkgs.mozjpeg}";
            NIX_ENFORCE_NO_NATIVE = "0";
          };
        });
    };
}
