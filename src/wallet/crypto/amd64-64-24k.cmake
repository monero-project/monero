include("${CMAKE_CURRENT_SOURCE_DIR}/amd64.cmake")
enable_language(ASM-ATT)
add_wallet_amd64_lib(
  "wallet-crypto-amd64-64-24k"
  "amd64-64-24k"
  "ed25519/amd64-64-24k"
  SOURCES
    "ed25519/amd64-64-24k/fe25519_add.s"
    "ed25519/amd64-64-24k/fe25519_sub.s"
)
list(APPEND WALLET_CRYPTO_LIBS "wallet-crypto-amd64-64-24k")
