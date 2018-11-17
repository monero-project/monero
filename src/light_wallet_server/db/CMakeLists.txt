set(lws-db_sources account.cpp data.cpp json.cpp storage.cpp string.cpp)
set(lws-db_headers account.h data.h fwd.h json.h storage.h string.h)

monero_add_library(light_wallet_server-db ${lws-db_sources} ${lws-db_headers})
target_link_libraries(light_wallet_server-db
    cryptonote_core
    lmdb_lib
    serialization-new
)
