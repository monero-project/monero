set(serialization-new_sources json_core.cpp json_error.cpp)
set(serialization-new_headers json_core.h json_error.h json_input.h json_output.h)

monero_add_library(serialization-new
    ${serialization-new_sources}
    ${serialization-new_headers}
)
target_link_libraries(serialization-new epee)
