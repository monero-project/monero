# Numista Coins agent handoff

## Required next work

The Numista testnet and asset-consensus changes have not yet been compiled or
run through their tests. Do not call the testnet operational or publish a
Windows executable until these steps have completed successfully:

1. In Codespaces, initialize the repository submodules and configure a clean
   release build with tests enabled.
2. Build the project and run the complete CTest suite, including the
   `numista_assets` unit tests. Fix failures caused by this fork, then repeat
   the full build and test commands. Report any unrelated baseline failures.
3. Check Numista testnet startup, fresh genesis creation, restart/reopen,
   version-17 activation, and asset-ledger rollback/reorganization behavior.
   Record that the test issuer private key is not in the repository.
4. Once the source build and tests pass, build the Windows x64 release
   executables using the project's Windows cross-compilation dependencies or
   a Windows build runner. Do not label a Linux binary or an untested build as
   the Windows release.
5. Verify the resulting PE files and run each executable's `--version` and
   `--help` checks. Confirm that a launch with no network option selects
   Numista testnet, uses the Numista data directory and ports, and does not
   use Monero seed peers. Confirm that Monero mainnet requires `--mainnet`.
6. Report exact build/test commands, outcomes, and the paths of the resulting
   Windows executables. Keep generated binaries and build trees out of source
   control unless explicitly requested.

## Build and test notes

- The project uses CMake and Ninja/Make; consult `README.md`, `Makefile`, and
  `contrib/depends` for current platform dependencies.
- CMake's `BUILD_TESTS` option defaults to `OFF`; explicitly enable it for
  validation. Run CTest from the configured build directory after building.
- Use a fresh build directory when changing compiler, generator, target
  platform, or toolchain. Do not reuse the existing `build/CMakeCache.txt`
  for a different toolchain.
- A successful Linux/Codespaces build does not produce or validate a Windows
  executable. The Windows target must be built and checked separately.

## Network and key safety

- Numista testnet is a separate network. Do not change Monero mainnet, testnet,
  or stagenet parameters to implement Numista behavior.
- The default daemon and wallet network is Numista testnet. Monero mainnet is
  explicitly selected with `--mainnet`.
- Keep the Numista testnet issuer public key consensus-fixed. Never commit or
  print the corresponding private scalar, and never reuse it on production.
- Production network parameters remain intentionally unset. Do not invent
  production genesis, activation, address-prefix, port, or issuer-key values.
- Asset RPC and wallet commands are not implemented. Do not describe the
  prototype as production-ready or as a complete NFT system.
