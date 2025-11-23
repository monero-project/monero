#pragma once

// Removed: #include <limits.h> as it is not used by the configuration macros.

// --- Output and Configuration Settings ---

// Defines the default log file path. Empty string means no default file logging
// and usually directs output to stdout/stderr unless custom handlers are used.
#define ELPP_DEFAULT_LOG_FILE ""

// Enables UTC (Coordinated Universal Time) instead of local time in timestamps.
#define ELPP_UTC_DATETIME


// --- Safety, Performance, and Platform Settings ---

// Enables thread safety (mutexes) for concurrent logging operations. Highly recommended.
#define ELPP_THREAD_SAFE

// Disables the default crash handling provided by EasyLogging++.
// This means the application must handle crashes or rely on OS/other libraries.
#define ELPP_DISABLE_DEFAULT_CRASH_HANDLING

// Disables internal checks done by ELPP macros (e.g., checks related to log levels). 
// Improves performance but sacrifices safety checks.
#define ELPP_NO_CHECK_MACROS

// Disables debug logging macros (e.g., CLOG, FDEBUG) entirely at compile time. 
// Improves performance and reduces binary size in release builds.
#define ELPP_NO_DEBUG_MACROS

// Enables support for Winsock2 (required for Windows networking, used in some ELPP features).
#define ELPP_WINSOCK2


// --- Platform-Specific Features (Crash Log) ---

// This complex condition determines if the crash log feature should be enabled.
// It is typically enabled unless running on specific C standard library/compiler combinations 
// that might have compatibility issues with ELPP's crash handler (e.g., some non-GNU/GLIBC platforms).

#ifdef EASYLOGGING_CC
#if !(!defined __GLIBC__ || !defined __GNUC__ || defined __MINGW32__ || defined __MINGW64__ || defined __ANDROID__ || defined __NetBSD__) || (defined __FreeBSD__)
// ELPP_FEATURE_CRASH_LOG enables the ability to write logs upon an application crash (e.g., signal handler).
#define ELPP_FEATURE_CRASH_LOG
#endif
#endif
