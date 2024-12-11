{ pkgs ? import <nixpkgs> { } }:

with pkgs;
mkShell {
  name = "soc";
  buildInputs = [
    gnumake
    meson
    ninja
    stdenv.cc

    doxygen
  ];
}
