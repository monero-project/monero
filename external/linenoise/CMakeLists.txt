project(linenoise C)

add_library(linenoise STATIC linenoise.c)
set_target_properties(linenoise PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(linenoise PROPERTIES C_STANDARD 11)

target_compile_definitions(linenoise PRIVATE _DEFAULT_SOURCE)

target_include_directories(linenoise PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
