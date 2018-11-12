#pragma once

#define ELPP_THREAD_SAFE
#define ELPP_DEFAULT_LOG_FILE ""
#define ELPP_DISABLE_DEFAULT_CRASH_HANDLING
#define ELPP_NO_CHECK_MACROS
#define ELPP_WINSOCK2
#define ELPP_NO_DEBUG_MACROS
#define ELPP_UTC_DATETIME

#ifdef EASYLOGGING_CC
#if !(!defined __GNUC__ || defined __MINGW32__ || defined __MINGW64__ || defined __ANDROID__)
#define ELPP_FEATURE_CRASH_LOG
#endif
#endif
