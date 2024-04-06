project(libqrcodegen)

add_library(qrcodegen STATIC QrCode.cpp)
set_target_properties(qrcodegen PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(qrcodegen PROPERTIES CXX_STANDARD 11)

target_include_directories(qrcodegen PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR})
