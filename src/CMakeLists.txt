add_definitions(-DSTATICLIB)

file(GLOB_RECURSE COMMON common/*)
file(GLOB_RECURSE CRYPTO crypto/*)
file(GLOB_RECURSE CRYPTONOTE_CORE cryptonote_core/*)
file(GLOB_RECURSE CRYPTONOTE_PROTOCOL cryptonote_protocol/*)
file(GLOB_RECURSE DAEMON daemon/*)
file(GLOB_RECURSE P2P p2p/*)
file(GLOB_RECURSE RPC rpc/*)
file(GLOB_RECURSE SIMPLEWALLET simplewallet/*)
file(GLOB_RECURSE CONN_TOOL connectivity_tool/*)
file(GLOB_RECURSE WALLET wallet/*)
file(GLOB_RECURSE MINER miner/*)
file(GLOB_RECURSE MONERO_WALLET_WRAPPER monero_wallet_wrapper/*)
file(GLOB_RECURSE MONERO_WALLET_WRAPPER_HEADERS monero_wallet_wrapper/*.hh)

source_group(common FILES ${COMMON})
source_group(crypto FILES ${CRYPTO})
source_group(cryptonote_core FILES ${CRYPTONOTE_CORE})
source_group(cryptonote_protocol FILES ${CRYPTONOTE_PROTOCOL})
source_group(daemon FILES ${DAEMON})
source_group(p2p FILES ${P2P})
source_group(rpc FILES ${RPC})
source_group(simplewallet FILES ${SIMPLEWALLET})
source_group(connectivity-tool FILES ${CONN_TOOL})
source_group(wallet FILES ${WALLET})
source_group(simpleminer FILES ${MINER})
source_group(monero_wallet_wrapper FILES ${MONERO_WALLET_WRAPPER})

add_library(common ${COMMON})
add_library(crypto ${CRYPTO})
add_library(cryptonote_core ${CRYPTONOTE_CORE})
add_executable(daemon ${DAEMON} ${P2P} ${CRYPTONOTE_PROTOCOL})
add_executable(connectivity_tool ${CONN_TOOL})
add_executable(simpleminer ${MINER})
target_link_libraries(daemon rpc cryptonote_core crypto common upnpc-static ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_link_libraries(connectivity_tool cryptonote_core crypto common ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_link_libraries(simpleminer cryptonote_core crypto common ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
add_library(rpc ${RPC})
add_library(wallet ${WALLET})
add_executable(simplewallet ${SIMPLEWALLET} )
target_link_libraries(simplewallet wallet rpc cryptonote_core crypto common upnpc-static ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
add_dependencies(daemon version)
add_dependencies(rpc version)
add_dependencies(simplewallet version)


add_library(monerowallet SHARED ${MONERO_WALLET_WRAPPER})
add_dependencies(monerowallet wallet)
target_link_libraries(monerowallet wallet cryptonote_core crypto common upnpc-static ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

#target_link_libraries(monerowallet wallet rpc cryptonote_core crypto common upnpc-static ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

set_property(TARGET common crypto cryptonote_core rpc wallet monerowallet PROPERTY FOLDER "libs")
set_property(TARGET daemon simplewallet connectivity_tool simpleminer PROPERTY FOLDER "prog")
set_property(TARGET daemon PROPERTY OUTPUT_NAME "bitmonerod")

INSTALL(TARGETS monerowallet
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

INSTALL(FILES ${MONERO_WALLET_WRAPPER_HEADERS} DESTINATION include/MoneroWallet)
