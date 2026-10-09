#[==[
@file vtkEncodeString.cmake

This module contains the @ref vtk_encode_string function which may be used to
turn a file into a C string. This is primarily used within a program so that
the content does not need to be retrieved from the filesystem at runtime, but
can still be developed as a standalone file.
#]==]

set(_vtkEncodeString_script_file "${CMAKE_CURRENT_LIST_FILE}")

#[==[
@brief Encode a file as a C string at build time

Adds a rule to turn a file into a C string. Note that any Unicode characters
will not be replaced with escaping, so it is recommended to avoid their usage
in the input.

~~~
vtk_encode_string(
  INPUT           <input>
  [NAME           <name>]
  [EXPORT_SYMBOL  <symbol>]
  [EXPORT_HEADER  <header>]
  [HEADER_OUTPUT  <variable>]
  [SOURCE_OUTPUT  <variable>]

  [ABI_MANGLE_SYMBOL_BEGIN <being>]
  [ABI_MANGLE_SYMBOL_END   <end>]
  [ABI_MANGLE_HEADER       <header>]

  [BINARY] [NUL_TERMINATE])
~~~

The only required variable is `INPUT`, however, it is likely that at least one
of `HEADER_OUTPUT` or `SOURCE_OUTPUT` will be required to add them to a
library.

  * `INPUT`: (Required) The path to the file to be embedded. If a relative path
    is given, it will be interpreted as being relative to
    `CMAKE_CURRENT_SOURCE_DIR`.
  * `NAME`: This is the base name of the files that will be generated as well
    as the variable name for the C string. It defaults to the basename of the
    input without extensions.
  * `EXPORT_SYMBOL`: The symbol to use for exporting the variable. By default,
    it will not be exported. If set, `EXPORT_HEADER` must also be set.
  * `EXPORT_HEADER`: The header to include for providing the given export
    symbol. If set, `EXPORT_SYMBOL` should also be set.
  * `HEADER_OUTPUT`: The variable to store the generated header path.
  * `SOURCE_OUTPUT`: The variable to store the generated source path.
  * `BINARY`: If given, the data will be written as an array of `unsigned char`
    bytes.
  * `NUL_TERMINATE`: If given, the binary data will be `NUL`-terminated. Only
    makes sense with the `BINARY` flag. This is intended to be used if
    embedding a file as a C string exceeds compiler limits on string literals
    in various compilers.
  * `ABI_MANGLE_SYMBOL_BEGIN`: Open a mangling namespace with the given symbol.
    If given, `ABI_MANGLE_SYMBOL_END` and `ABI_MANGLE_HEADER` must also be set.
  * `ABI_MANGLE_SYMBOL_END`: Close a mangling namespace with the given symbol.
    If given, `ABI_MANGLE_SYMBOL_BEGIN` and `ABI_MANGLE_HEADER` must also be set.
  * `ABI_MANGLE_HEADER`: The header which provides the ABI mangling symbols.
    If given, `ABI_MANGLE_SYMBOL_BEGIN` and `ABI_MANGLE_SYMBOL_END` must also
    be set.
#]==]
function (vtk_encode_string)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_encode_string
    "BINARY;NUL_TERMINATE"
    "INPUT;NAME;EXPORT_SYMBOL;EXPORT_HEADER;HEADER_OUTPUT;SOURCE_OUTPUT;ABI_MANGLE_SYMBOL_BEGIN;ABI_MANGLE_SYMBOL_END;ABI_MANGLE_HEADER"
    "")

  if (_vtk_encode_string_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unrecognized arguments to vtk_encode_string: "
      "${_vtk_encode_string_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_encode_string_INPUT)
    message(FATAL_ERROR
      "Missing `INPUT` for vtk_encode_string.")
  endif ()

  if (NOT DEFINED _vtk_encode_string_NAME)
    get_filename_component(_vtk_encode_string_NAME
      "${_vtk_encode_string_INPUT}" NAME_WE)
  endif ()

  if (DEFINED _vtk_encode_string_EXPORT_SYMBOL AND
      NOT DEFINED _vtk_encode_string_EXPORT_HEADER)
    message(FATAL_ERROR
      "Missing `EXPORT_HEADER` when using `EXPORT_SYMBOL`.")
  endif ()

  if (DEFINED _vtk_encode_string_EXPORT_HEADER AND
      NOT DEFINED _vtk_encode_string_EXPORT_SYMBOL)
    message(WARNING
      "Missing `EXPORT_SYMBOL` when using `EXPORT_HEADER`.")
  endif ()

  if (DEFINED _vtk_encode_string_ABI_MANGLE_SYMBOL_BEGIN AND
      (NOT DEFINED _vtk_encode_string_ABI_MANGLE_SYMBOL_END OR
       NOT DEFINED _vtk_encode_string_ABI_MANGLE_HEADER))
    message(WARNING
      "Missing `ABI_MANGLE_SYMBOL_END` or `ABI_MANGLE_HEADER` when using "
      "`ABI_MANGLE_SYMBOL_BEGIN`.")
  endif ()

  if (DEFINED _vtk_encode_string_ABI_MANGLE_SYMBOL_END AND
      (NOT DEFINED _vtk_encode_string_ABI_MANGLE_SYMBOL_BEGIN OR
       NOT DEFINED _vtk_encode_string_ABI_MANGLE_HEADER))
    message(WARNING
      "Missing `ABI_MANGLE_SYMBOL_BEGIN` or `ABI_MANGLE_HEADER` when using "
      "`ABI_MANGLE_SYMBOL_END`.")
  endif ()

  if (DEFINED _vtk_encode_string_ABI_MANGLE_HEADER AND
      (NOT DEFINED _vtk_encode_string_ABI_MANGLE_SYMBOL_BEGIN OR
       NOT DEFINED _vtk_encode_string_ABI_MANGLE_SYMBOL_END))
    message(WARNING
      "Missing `ABI_MANGLE_SYMBOL_BEGIN` or `ABI_MANGLE_SYMBOL_END` when "
      "using `ABI_MANGLE_HEADER`.")
  endif ()

  if (NOT _vtk_encode_string_BINARY AND _vtk_encode_string_NUL_TERMINATE)
    message(FATAL_ERROR
      "The `NUL_TERMINATE` flag only makes sense with the `BINARY` flag.")
  endif ()

  set(_vtk_encode_string_header
    "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_encode_string_NAME}.h")
  set(_vtk_encode_string_source
    "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_encode_string_NAME}.cxx")

  if (IS_ABSOLUTE "${_vtk_encode_string_INPUT}")
    set(_vtk_encode_string_input
      "${_vtk_encode_string_INPUT}")
  else ()
    set(_vtk_encode_string_input
      "${CMAKE_CURRENT_SOURCE_DIR}/${_vtk_encode_string_INPUT}")
  endif ()

  set(_vtk_encode_string_depends_args)
  if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
    list(APPEND _vtk_encode_string_depends_args
      DEPENDS_EXPLICIT_ONLY)
  endif ()

  add_custom_command(
    OUTPUT  ${_vtk_encode_string_header}
            ${_vtk_encode_string_source}
    DEPENDS "${_vtkEncodeString_script_file}"
            "${_vtk_encode_string_input}"
    COMMAND "${CMAKE_COMMAND}"
            "-Dsource_dir=${CMAKE_CURRENT_SOURCE_DIR}"
            "-Dbinary_dir=${CMAKE_CURRENT_BINARY_DIR}"
            "-Dsource_file=${_vtk_encode_string_input}"
            "-Doutput_name=${_vtk_encode_string_NAME}"
            "-Dexport_symbol=${_vtk_encode_string_EXPORT_SYMBOL}"
            "-Dexport_header=${_vtk_encode_string_EXPORT_HEADER}"
            "-Dabi_mangle_symbol_begin=${_vtk_encode_string_ABI_MANGLE_SYMBOL_BEGIN}"
            "-Dabi_mangle_symbol_end=${_vtk_encode_string_ABI_MANGLE_SYMBOL_END}"
            "-Dabi_mangle_header=${_vtk_encode_string_ABI_MANGLE_HEADER}"
            "-Dbinary=${_vtk_encode_string_BINARY}"
            "-Dnul_terminate=${_vtk_encode_string_NUL_TERMINATE}"
            "-D_vtk_encode_string_run=ON"
            -P "${_vtkEncodeString_script_file}"
    ${_vtk_encode_string_depends_args})

  if (DEFINED _vtk_encode_string_SOURCE_OUTPUT)
    set("${_vtk_encode_string_SOURCE_OUTPUT}"
      "${_vtk_encode_string_source}"
      PARENT_SCOPE)
  endif ()

  if (DEFINED _vtk_encode_string_HEADER_OUTPUT)
    set("${_vtk_encode_string_HEADER_OUTPUT}"
      "${_vtk_encode_string_header}"
      PARENT_SCOPE)
  endif ()
endfunction ()

if (_vtk_encode_string_run AND CMAKE_SCRIPT_MODE_FILE)
  set(output_header "${binary_dir}/${output_name}.h")
  set(output_source "${binary_dir}/${output_name}.cxx")

  set(license_topfile "// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen\n// SPDX-License-Identifier: BSD-3-Clause\n")
  file(WRITE "${output_header}" ${license_topfile})
  file(WRITE "${output_source}" ${license_topfile})

  file(APPEND "${output_header}"
    "#ifndef ${output_name}_h\n#define ${output_name}_h\n\n")
  if (export_header)
    file(APPEND "${output_header}"
      "#include \"${export_header}\"\n")
  endif ()
  if (abi_mangle_header AND abi_mangle_symbol_begin)
    file(APPEND "${output_header}"
      "#include \"${abi_mangle_header}\"\n\n${abi_mangle_symbol_begin}\n\n")
  endif ()
  if (export_symbol)
    file(APPEND "${output_header}"
      "${export_symbol} ")
  endif ()

  if (IS_ABSOLUTE "${source_file}")
    set(source_file_full "${source_file}")
  else ()
    set(source_file_full "${source_dir}/${source_file}")
  endif ()
  set(hex_arg)
  if (binary)
    set(hex_arg HEX)
  endif ()
  file(READ "${source_file_full}" original_content ${hex_arg})

  if (binary)
    if (nul_terminate)
      string(APPEND original_content "00")
    endif ()
    string(LENGTH "${original_content}" output_size)
    math(EXPR output_size "${output_size} / 2")
    file(APPEND "${output_header}"
      "extern const unsigned char ${output_name}[${output_size}];\n\n")
    if (abi_mangle_symbol_end)
      file(APPEND "${output_header}"
        "${abi_mangle_symbol_end}\n")
    endif ()
    file(APPEND "${output_header}"
      "#endif\n")

    file(APPEND "${output_source}"
      "#include \"${output_name}.h\"\n\n")
    if (abi_mangle_symbol_begin)
      file(APPEND "${output_source}"
        "${abi_mangle_symbol_begin}\n\n")
    endif ()
    file(APPEND "${output_source}"
      "const unsigned char ${output_name}[${output_size}] = {\n")
    string(REGEX REPLACE "\([0-9a-f][0-9a-f]\)" ",0x\\1" escaped_content "${original_content}")
    # Hard line wrap the file.
    string(REGEX REPLACE "\(..........................................................................,\)" "\\1\n" escaped_content "${escaped_content}")
    # Remove the leading comma.
    string(REGEX REPLACE "^," "" escaped_content "${escaped_content}")
    file(APPEND "${output_source}"
      "${escaped_content}\n")
    file(APPEND "${output_source}"
      "};\n")
    if (abi_mangle_symbol_end)
      file(APPEND "${output_source}"
        "${abi_mangle_symbol_end}\n")
    endif ()
  else ()
    file(APPEND "${output_header}"
      "extern const char *${output_name};\n\n")
    if (abi_mangle_symbol_end)
      file(APPEND "${output_header}"
        "${abi_mangle_symbol_end}\n\n")
    endif ()
    file(APPEND "${output_header}"
      "#endif\n")

    # Escape literal backslashes.
    string(REPLACE "\\" "\\\\" escaped_content "${original_content}")
    # Escape literal double quotes.
    string(REPLACE "\"" "\\\"" escaped_content "${escaped_content}")
    # Turn newlines into newlines in the C string.
    string(REPLACE "\n" "\\n\"\n\"" escaped_content "${escaped_content}")

    file(APPEND "${output_source}"
      "#include \"${output_name}.h\"\n\n")
    if (abi_mangle_symbol_begin)
      file(APPEND "${output_source}"
        "${abi_mangle_symbol_begin}\n\n")
    endif ()
    file(APPEND "${output_source}"
      "const char *${output_name} =\n")
    file(APPEND "${output_source}"
      "\"${escaped_content}\";\n")
    if (abi_mangle_symbol_end)
      file(APPEND "${output_source}"
        "\n${abi_mangle_symbol_end}\n")
    endif ()
  endif ()
endif ()
