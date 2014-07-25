#pragma once

#if defined(__GNUC__)
  #define PRAGMA_WARNING_PUSH _Pragma("GCC diagnostic push")
  #define PRAGMA_WARNING_POP _Pragma("GCC diagnostic pop")
  #define PRAGMA_WARNING_DISABLE_VS(w)
  #define PRAGMA_GCC(w) _Pragma(w)
#elif defined(_MSC_VER)
  #define PRAGMA_WARNING_PUSH __pragma(warning( push ))
  #define PRAGMA_WARNING_POP __pragma(warning( pop ))
  #define PRAGMA_WARNING_DISABLE_VS(w) __pragma( warning ( disable: w ))
  //#define PRAGMA_WARNING_DISABLE_GCC(w)
  #define PRAGMA_GCC(w)
#endif
