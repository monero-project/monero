
add_subdirectory(db)

# For both the server and admin utility.
set(lws_common_sources config.cpp error.cpp json.cpp)
set(lws_common_headers config.h error.h fwd.h json.h)

monero_add_library(light_wallet_server-common ${lws_common_sources} ${lws_common_headers})
target_link_libraries(light_wallet_server-common common serialization-new)

# Only used by server (not admin). For utilities that are unit tested
set(lws_lib_sources random_outputs.cpp)
set(lws_lib_headers random_outputs.h)

monero_add_library(light_wallet_server-lib ${lws_lib_sources} ${lws_lib_headers})
target_link_libraries(light_wallet_server-lib light_wallet_server-common wallet)

monero_add_executable(light_wallet_server server_main.cpp rates.cpp rest_server.cpp rpc.cpp scanner.cpp)
target_include_directories(light_wallet_server PUBLIC ${ZMQ_INCLUDE_PATH})
target_link_libraries(light_wallet_server
  PRIVATE
    cryptonote_core
    cncrypto
    common
    daemon_messages
    device
    epee
    light_wallet_server-common
    light_wallet_server-db
    light_wallet_server-lib
    lmdb_lib
    daemon_messages
    ${Boost_CHRONO_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES}
    ${ZMQ_LIB}
)
set_property(TARGET light_wallet_server PROPERTY OUTPUT_NAME "monero-light-wallet-server")

monero_add_executable(light_wallet_server_admin admin_main.cpp)
target_link_libraries(light_wallet_server_admin
  PRIVATE
    cryptonote_core
    cncrypto
    common
    light_wallet_server-common
    light_wallet_server-db
    serialization-new
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${EXTRA_LIBRARIES}
)
set_property(TARGET light_wallet_server_admin PROPERTY OUTPUT_NAME "monero-light-wallet-server-admin")

install(TARGETS light_wallet_server DESTINATION bin)
install(TARGETS light_wallet_server_admin DESTINATION bin)
