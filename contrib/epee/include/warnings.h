#pragma once

#if defined(_MSC_VER)

#define PUSH_WARNINGS __pragma(warning(push))
#define POP_WARNINGS __pragma(warning(pop))
#define DISABLE_VS_WARNINGS(w) __pragma(warning(disable: w))
#define DISABLE_GCC_WARNING(w)
#define DISABLE_CLANG_WARNING(w)
#define DISABLE_GCC_AND_CLANG_WARNING(w)

#else

#include <boost/preprocessor/stringize.hpp>

#define PUSH_WARNINGS _Pragma("GCC diagnostic push")
#define POP_WARNINGS _Pragma("GCC diagnostic pop")
#define DISABLE_VS_WARNINGS(w)

#if defined(__clang__)
#define DISABLE_GCC_WARNING(w)
#define DISABLE_CLANG_WARNING DISABLE_GCC_AND_CLANG_WARNING
#else
#define DISABLE_GCC_WARNING DISABLE_GCC_AND_CLANG_WARNING
#define DISABLE_CLANG_WARNING(w)
#endif

#define DISABLE_GCC_AND_CLANG_WARNING(w) _Pragma(BOOST_PP_STRINGIZE(GCC diagnostic ignored BOOST_PP_STRINGIZE(-W##w)))

#endif
