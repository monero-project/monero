

function(add_wallet_amd64_lib LIBNAME FILENAME LIBFOLDER)
  set(MULTIVARS SOURCES DIRECTORIES)
  cmake_parse_arguments(AMD64_PERF "" "" "${MULTIVARS}" ${ARGN})

  enable_language(ASM-ATT)
  add_library(${LIBNAME}
    "${FILENAME}.h" "${FILENAME}.c" "${FILENAME}-choose_tp.s"
    "${LIBFOLDER}/choose_t.s"
    "${LIBFOLDER}/consts.s"
    "${LIBFOLDER}/fe25519_getparity.c"
    "${LIBFOLDER}/fe25519_freeze.s"
    "${LIBFOLDER}/fe25519_invert.c"
    "${LIBFOLDER}/fe25519_iseq.c"
    "${LIBFOLDER}/fe25519_mul.s"
    "${LIBFOLDER}/fe25519_neg.c"
    "${LIBFOLDER}/fe25519_pack.c"
    "${LIBFOLDER}/fe25519_pow2523.c"
    "${LIBFOLDER}/fe25519_setint.c"
    "${LIBFOLDER}/fe25519_square.s"
    "${LIBFOLDER}/fe25519_unpack.c"
    "${LIBFOLDER}/ge25519_add.c"
    "${LIBFOLDER}/ge25519_add_p1p1.s"
    "${LIBFOLDER}/ge25519_dbl_p1p1.s"
    "${LIBFOLDER}/ge25519_double.c"
    "${LIBFOLDER}/ge25519_nielsadd_p1p1.s"
    "${LIBFOLDER}/ge25519_nielsadd2.s"
    "${LIBFOLDER}/ge25519_pack.c"
    "${LIBFOLDER}/ge25519_p1p1_to_p2.s"
    "${LIBFOLDER}/ge25519_p1p1_to_p3.s"
    "${LIBFOLDER}/ge25519_pnielsadd_p1p1.s"
    "${LIBFOLDER}/ge25519_scalarmult_base.c"
    "${LIBFOLDER}/ge25519_unpackneg.c"
    "${LIBFOLDER}/sc25519_from32bytes.c"
    "${LIBFOLDER}/sc25519_window4.c"
    ${AMD64_PERF_SOURCES}
  ) 
  target_include_directories(${LIBNAME} PRIVATE ${LIBFOLDER} ${AMD64_PERF_DIRECTORIES})
endfunction (add_wallet_amd64_lib)
