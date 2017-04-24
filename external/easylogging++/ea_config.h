#pragma once

#define ELPP_THREAD_SAFE
#define ELPP_DEFAULT_LOG_FILE ""
#if !defined __GNUC__ || defined __MINGW32__ || defined __MINGW64__ || defined __ANDROID__
#else
#define ELPP_FEATURE_CRASH_LOG 1
#endif
#define ELPP_DISABLE_DEFAULT_CRASH_HANDLING
#define ELPP_NO_CHECK_MACROS
