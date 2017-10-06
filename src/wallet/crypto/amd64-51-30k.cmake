include("${CMAKE_CURRENT_SOURCE_DIR}/amd64.cmake")
enable_language(ASM-ATT)
add_wallet_amd64_lib(
  "wallet-crypto-amd64-51-30k"
  "amd64-51-30k"
  "ed25519/amd64-51-30k"
  SOURCES
    "ed25519/amd64-51-30k/fe25519_add.c"
    "ed25519/amd64-51-30k/fe25519_nsquare.s"
    "ed25519/amd64-51-30k/fe25519_sub.c"
    "ed25519/amd64-51-30k/ge25519_p1p1_to_pniels.s"
)
list(APPEND WALLET_CRYPTO_LIBS "wallet-crypto-amd64-51-30k")
