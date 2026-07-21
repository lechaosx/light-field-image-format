{
  description = "Light Field Image Format";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = {
    self,
    nixpkgs,
  }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    devShells.${system}.default = (pkgs.mkShell.override {stdenv = pkgs.gcc16Stdenv;}) {
      nativeBuildInputs = [
        pkgs.cmake
        pkgs.ninja
        pkgs.conan
        pkgs.llvmPackages_22.clang-tools
        pkgs.gdb
      ];
    };
  };
}
