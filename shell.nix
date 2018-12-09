let
  nixpkgs = import <nixpkgs> {};
  make-monero = nixpkgs.writeShellScriptBin "make-monero" "make -j$NIX_BUILD_CORES";
in
with nixpkgs;
stdenv.mkDerivation rec {
  name = "monero-build-environment";

  buildInputs = [
    cmake
    git
    gcc
    pkgconfig
    boost
    openssl
    zeromq3
    cppzmq
    unbound
    libsodium
    # needed for ledger nano s
    hidapi

    make-monero
  ];
}
