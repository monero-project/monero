project(libqrcodegen)

add_library(qrcodegen STATIC QrCode.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

target_include_directories(qrcodegen PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR})
