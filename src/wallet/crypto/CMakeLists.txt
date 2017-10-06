
set(WALLET_CRYPTO_CHOICES "none" "auto" "amd64-51-30k" "amd64-64-24k")
set(WALLET_CRYPTO "auto" CACHE STRING "Select a wallet import performance mode")

if (${WALLET_CRYPTO} STREQUAL "auto")
  if (UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|AMD64.*|x86_64.*")
    message("Wallet import performance - using \"amd64-51-30k\"")
    set(WALLET_CRYPTO "amd64-51-30k")
  else ()
    message("Wallet import performance disabled - no improvements for platform")
    set(WALLET_CRYPTO "none")
  endif ()
endif ()

list(FIND WALLET_CRYPTO_CHOICES ${WALLET_CRYPTO} CHOICE_FOUND)
if (CHOICE_FOUND LESS 0)
  message(FATAL_ERROR "Invalid WALLET_CRYPTO option ${WALLET_CRYPTO}")
endif ()

string(REPLACE "-" "_" WALLET_CRYPTO_NAMESPACE "${WALLET_CRYPTO}")
if (NOT ${WALLET_CRYPTO} STREQUAL "none")
  include("${CMAKE_CURRENT_SOURCE_DIR}/${WALLET_CRYPTO}.cmake")
endif ()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/import.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/import.h")
add_library(wallet-crypto INTERFACE)
if (WALLET_CRYPTO_LIBS)
  target_link_libraries(wallet-crypto INTERFACE ${WALLET_CRYPTO_LIBS})
endif ()

