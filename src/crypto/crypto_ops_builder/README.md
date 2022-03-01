# Monero

Copyright (c) 2014-2022, The Monero Project

## Crypto Ops Builder

In order to ensure the safest implementation of the cryptography in use by Monero we have opted to use the SUPERCOP ref10 implementations wherever possible. The main reason is that SUPERCOP ref10 is old, well tested, and primarily the work of Daniel J. Bernstein and Tanja Lange (among others, see ```designers``` in the ref10 folder). This is particularly relevant, as the team that designed Curve25519 and EdDSA, both of which are at Monero's core, is the same team that created the SUPERCOP implementation.

SUPERCOP ref10 is a fairly secure implementation that focuses on things like constant-time algorithms, to reduce side-channel attacks, sometimes at the cost of performance. However, we consider this a fair trade-off, especially considering that Monero is not that performance sensitive at this stage. In future we may consider faster implementations that still have a measure of safety against side-channel attacks.

## Additional Cryptography

Unfortunately SUPERCOP ref10 does not contain every function Monero's ```crypto-ops``` class needs. Thus there are several new files in the ```ref10CommentedCombined``` folder which allow for the class to be built during compilation. The original ref10 is included in the source tree in order to allow for a comparison to be made between the two, and also to allow for a quick comparison to be made between our in-source copy of SUPERCOP ref10 and an independently downloaded copy.

## Usage

The operation to produce the ```crypto-ops.c``` is automatic and part of the build process. If, however, you want to manually run the build process to verify the output, you can use ```MakeCryptoOps.py```.

## Attribution

The majority of the work we are using is from SUPERCOP, and copyrights and attribution fall to those developers and cryptographers. Beyond that we also include some of the original CryptoNote reference code. The entire build process, and all of the work analysing the functions and figuring out what comes from where, has been done by the Monero Research Lab. Shen Noether, in particular, deserves the bulk of the attribution for that.
