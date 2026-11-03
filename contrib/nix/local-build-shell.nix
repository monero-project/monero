# Refer to README.md for how to invoke this.

{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  nativeBuildInputs = with pkgs.buildPackages; [
    autoconf
    cmake
    git
    libsodium
    libunwind
    openssl
    protobuf
    python3
    readline
    unbound
    zeromq
  ]  ++ lib.optionals stdenv.isDarwin [
    darwin.apple_sdk.frameworks.IOKit
  ];

  buildInputs = with pkgs.buildPackages; [
    boost
  ];

  cmakeFlags = [
    "-DReadline_ROOT_DIR=${pkgs.buildPackages.readline.dev}"
  ];
}
