set(polyseed_sources
        pbkdf2.c
        polyseed.cpp
)

monero_find_all_headers(polyseed_private_headers "${CMAKE_CURRENT_SOURCE_DIR}")

monero_private_headers(polyseed_wrapper
        ${polyseed_private_headers}
)

monero_add_library(polyseed_wrapper
        ${polyseed_sources}
        ${polyseed_headers}
        ${polyseed_private_headers}
)

target_link_libraries(polyseed_wrapper
PUBLIC
        polyseed
        cryptonote_basic
        utf8proc
        sodium
PRIVATE
        ${EXTRA_LIBRARIES}
)
