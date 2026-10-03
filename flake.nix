{
  description = "Monero development environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs = { nixpkgs, ... }:
    let
      forAllSystems = nixpkgs.lib.genAttrs [ "x86_64-linux" "aarch64-linux" ];
    in {
      devShells = forAllSystems (system:
        let pkgs = nixpkgs.legacyPackages.${system};
        in {
          default = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [ cmake pkg-config git rustc cargo ];
            buildInputs = with pkgs; [
              boost openssl zeromq unbound libsodium hidapi libusb1
              protobuf readline expat zlib
            ] ++ pkgs.lib.optional pkgs.stdenv.hostPlatform.isLinux pkgs.libunwind;
          };
        });
    };
}
