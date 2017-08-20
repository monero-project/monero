# Monero Wallet Performance Crypto
## Usage

Monero wallet import performance is toggled with `-DWALLET_CRYPTO=`. The
following options are valid strings for the option:
  - *`none`* uses the standard default monero (ref10) implementation
  - *`auto`* uses `amd64-51-30k` when targeting amd64, and `none` otherwise.
    This is the default.
  - *`amd64-51-30k`* uses amd64-51-30k code from supercop
  - *`amd64-64-24k`* uses amd64-64-24k code from supercop

##  How it Works

CMake generates `src/wallet/crypto/import.h` based on user configuration. The
header file always _aliases_ `generate_key_derivation` and `derive_public_key`
in the `tools::wallet_only` namespace. So when the performance code is disabled,
the default functions are called (via aliasing) for identical performance and
behavior. If performance is enabled, it aliases the enabled function. This
_could_ allow multiple performance implementations to be simultaneously
compiled into the program (in fact it was designed this way). The original
crypto functions are *never* modified.

## Implementations
### `amd64-51-30k`

The code from supercop (`src/wallet/crypto/ed25519/amd64-51-30k`) is untouched
with the exception that it is made position independent. Additionally, the
following additions were "necessary":
  - *`unpack_vartime`* - the supercop version automatically negates the x-pos.
  - *`monero_scalarmult`* - the supercop version does not do strict ECDH - only
    base multiplication and double (i.e. a*B + c*G).
  - *`crypto_sign_ed25519_amd64_51_30k_batch_choose_tp`* - the default version
    did not use `z` for space savings. This meant multiple inversions in the
    pre-compuation stage OR a non-constant adaption of the double scalar code.
    The first option was implemented - a modified ASM that "selects" the `z`
    from the pre-computation stage so that only a single inversion is done at
    the end and constant time behavior is maintained.

### `amd64-64-24k`

The code from supercop (`src/wallet/crypto/ed25519/amd64-64-24k`) is completely
with the exception that is it made position independent. See
(amd64-51-30 section)[#amd64-51-30k] for other changes made.

## Future Directions
### Scalar Multiplication

It _might_ be faster to convert to the montgomery curve to use supercop
scalarmult code for the ECDH step, but the performance could be tight since it
has to convert back to edwards curve after completion. Lots of investigation is
needed, because the conversion back is going to need y-point recovery (all of
the code uses `X/Z` only for computing the ECDH).
