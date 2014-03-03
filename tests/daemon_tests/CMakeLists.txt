add_executable(transfers transfers.cpp)
target_link_libraries(transfers useragent rpc cryptonote_core crypto common epee gtest_main ${Boost_LIBRARIES})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_transfers)
add_custom_target(test_transfers COMMAND transfers WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_transfers)
