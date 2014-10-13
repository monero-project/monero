#ifndef _GLIM_EXCEPTION_HPP_INCLUDED
#define _GLIM_EXCEPTION_HPP_INCLUDED

/// \file
/// Exceptions with configurable behaviour.
/// Requires `thread_local` support introduced in [gcc-4.8](http://gcc.gnu.org/gcc-4.8/changes.html)
/// (`__thread` is not reliable with GCC 4.7.2 across shared libraries).

#include <stdexcept>
#include <string>
#include <sstream>
#include <stdint.h>
#include <stdlib.h> // free
#include <unistd.h> // write

/// Throws `::glim::Exception` passing the current file and line into constructor.
#define GTHROW(message) throw ::glim::Exception (message, __FILE__, __LINE__)
/// Throws a `::glim::Exception` derived exception `name` passing the current file and line into constructor.
#define GNTHROW(name, message) throw name (message, __FILE__, __LINE__)
/// Helps defining new `::glim::Exception`-based exceptions.
/// Named exceptions might be useful in a debugger.
#define G_DEFINE_EXCEPTION(name) \
  struct name: public ::glim::Exception { \
    name (const ::std::string& message, const char* file, int line): ::glim::Exception (message, file, line) {} \
  }

// Workaround to compile under GCC 4.7.
#if defined (__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ == 7 && !defined (thread_local)
# define thread_local __thread
#endif

namespace glim {

// Ideas:
// RAII control via thread-local integer (with bits): option to capture stack trace (printed on "what()")
// see http://stacktrace.svn.sourceforge.net/viewvc/stacktrace/stacktrace/call_stack_gcc.cpp?revision=40&view=markup
// A handler to log exception with VALGRIND (with optional trace)
// A handler to log thread id and *pause* the thread in exception constructor (user can attach GDB and investigate)
// (or we might call an empty function: "I once used something similar,
//  but with an empty function debug_breakpoint. When debugging, I simply entered "bre debug_breakpoint"
//  at the gdb prompt - no asembler needed (compile debug_breakpoint in a separate compilation unit to avoid having the call optimized away)."
//  - http://stackoverflow.com/a/4720403/257568)
// A handler to call a debugger? (see: http://stackoverflow.com/a/4732119/257568)

// todo: Try a helper which uses cairo's backtrace-symbols.c
// http://code.ohloh.net/file?fid=zUOUdEl-Id-ijyPOmCkVnBJt2d8&cid=zGpizbyIjEw&s=addr2line&browser=Default#L7

// todo: Try a helper which uses cairo's lookup-symbol.c
// http://code.ohloh.net/file?fid=Je2jZqsOxge_SvWVrvywn2I0TIs&cid=zGpizbyIjEw&s=addr2line&browser=Default#L0

// todo: A helper converting backtrace to addr2line invocation, e.g.
// bin/test_exception() [0x4020cc];bin/test_exception(__cxa_throw+0x47) [0x402277];bin/test_exception() [0x401c06];/lib/x86_64-linux-gnu/libc.so.6(__libc_start_main+0xfd) [0x57f0ead];bin/test_exception() [0x401fd1];
// should be converted to
// addr2line -pifCa -e bin/test_exception 0x4020cc 0x402277 0x401c06 0x57f0ead 0x401fd1
//
// The helper should read the shared library addresses from /proc/.../map and generate separate addr2line invocations
// for groups of addresses inside the same shared library.
// => dladdr instead of /proc/../map; http://stackoverflow.com/a/2606152/257568
//
// Shared libraries (http://stackoverflow.com/a/7557756/257568).
// Example, backtrace: /usr/local/lib/libfrople.so(_ZN5mongo14BSONObjBuilder8appendAsERKNS_11BSONElementERKNS_10StringDataE+0x1ca) [0x2aef5b45eb8a]
// cat /proc/23630/maps | grep libfrople
//   -> 2aef5b363000-2aef5b53e000
// 0x2aef5b45eb8a - 2aef5b363000 = FBB8A
// addr2line -pifCa -e /usr/local/lib/libfrople.so 0xFBB8A
//
// cat /proc/`pidof FropleAndImg2`/maps | grep libfrople
// addr2line -pifCa -e /usr/local/lib/libfrople.so `perl -e 'printf ("%x", 0x2aef5b45eb8a - 0x2aef5b363000)'`

inline void captureBacktrace (void* stdStringPtr);

typedef void (*exception_handler_fn)(void*);

/// Exception with file and line information and optional stack trace capture.
/// Requires `thread_local` support ([gcc-4.8](http://gcc.gnu.org/gcc-4.8/changes.html)).
class Exception: public std::runtime_error {
 protected:
  const char* _file; int32_t _line;
  std::string _what;
  uint32_t _options;

  /// Append [{file}:{line}] into `buf`.
  void appendLine (std::string& buf) const {
    if (_file || _line > 0) {
      std::ostringstream oss;
      oss << '[';
      if (_file) oss << _file;
      if (_line >= 0) oss << ':' << _line;
      oss << "] ";
      buf.append (oss.str());
    }
  }

  /// Append a stack trace to `_what`.
  void capture() {
    if (_options & RENDEZVOUS) rendezvous();
    if (_options & CAPTURE_TRACE) {
      appendLine (_what);
      _what += "[at ";
      captureBacktrace (&_what);
      _what.append ("] ");
      _what += std::runtime_error::what();
    }
  }

 public:
  /** The reference to the thread-local options. */
  inline static uint32_t& options() {
    static thread_local uint32_t EXCEPTION_OPTIONS = 0;
    return EXCEPTION_OPTIONS;
  }
  enum Options: uint32_t {
    PLAIN_WHAT = 1,  ///< Pass `what` as is, do not add any information to it.
    HANDLE_ALL = 1 << 1,  ///< Run the custom handler from `__cxa_throw`.
    CAPTURE_TRACE = 1 << 2,  ///< Append a stack trace into the `Exception::_what` (with the help of the `captureBacktrace`).
    RENDEZVOUS = 1 << 3  ///< Call the rendezvous function in `throw` and in `what`, so that the GDB can catch it (break glim::Exception::rendezvous).
  };

  /** The pointer to the thread-local exception handler. */
  inline static exception_handler_fn* handler() {
    static thread_local exception_handler_fn EXCEPTION_HANDLER = nullptr;
    return &EXCEPTION_HANDLER;
  }
  /** The pointer to the thread-local argument for the exception handler. */
  inline static void** handlerArg() {
    static thread_local void* EXCEPTION_HANDLER_ARG = nullptr;
    return &EXCEPTION_HANDLER_ARG;
  }

  /// Invoked when the `RENDEZVOUS` option is set in order to help the debugger catch the exception (break glim::Exception::rendezvous).
  static void rendezvous() __attribute__((noinline)) {
    asm ("");  // Prevents the function from being optimized away.
  }

  Exception (const std::string& message):
    std::runtime_error (message), _file (0), _line (-1), _options (options()) {
    capture();}
  Exception (const std::string& message, const char* file, int32_t line):
    std::runtime_error (message), _file (file), _line (line), _options (options()) {
    capture();}
  ~Exception() throw() {}
  virtual const char* what() const throw() {
    if (_options & RENDEZVOUS) rendezvous();
    if (_options & PLAIN_WHAT) return std::runtime_error::what();
    std::string& buf = const_cast<std::string&> (_what);
    if (buf.empty()) {
      appendLine (buf);
      buf.append (std::runtime_error::what());
    }
    return buf.c_str();
  }
};

/// RAII control of thrown `Exception`s.
/// Example: \code
///   glim::ExceptionControl trace (glim::Exception::Options::CAPTURE_TRACE);
/// \endcode
/// Modifies the `Exception` options via a thread-local variable and restores them back upon destruction.\n
/// Currently uses http://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Thread_002dLocal.html
/// (might use C++11 `thread_local` in the future).
class ExceptionControl {
 protected:
  uint32_t _savedOptions;
 public:
  ExceptionControl (uint32_t newOptions) {
    uint32_t& options = Exception::options();
    _savedOptions = options;
    options = newOptions;
  }
  ~ExceptionControl() {
    Exception::options() = _savedOptions;
  }
};

class ExceptionHandler {
protected:
  uint32_t _savedOptions;
  exception_handler_fn _savedHandler;
  void* _savedHandlerArg;
public:
  ExceptionHandler (uint32_t newOptions, exception_handler_fn handler, void* handlerArg) {
    uint32_t& options = Exception::options(); _savedOptions = options; options = newOptions;
    exception_handler_fn* handler_ = Exception::handler(); _savedHandler = *handler_; *handler_ = handler;
    void** handlerArg_ = Exception::handlerArg(); _savedHandlerArg = *handlerArg_; *handlerArg_ = handlerArg;
  }
  ~ExceptionHandler() {
    Exception::options() = _savedOptions;
    *Exception::handler() = _savedHandler;
    *Exception::handlerArg() = _savedHandlerArg;
  }
};

} // namespace glim

#if defined(__GNUC__) && (defined (__linux__) || defined (_SYSTYPE_BSD))
# include <execinfo.h> // backtrace; http://www.gnu.org/software/libc/manual/html_node/Backtraces.html
# define _GLIM_USE_EXECINFO
#endif

namespace glim {

/** If `stdStringPtr` is not null then backtrace is saved there (must point to an std::string instance),
 * otherwise printed to write(2). */
void captureBacktrace (void* stdStringPtr) {
#ifdef _GLIM_USE_EXECINFO
  const int arraySize = 10; void *array[arraySize];
  int got = ::backtrace (array, arraySize);
  if (stdStringPtr) {
    std::string* out = (std::string*) stdStringPtr;
    char **strings = ::backtrace_symbols (array, got);
    for (int tn = 0; tn < got; ++tn) {out->append (strings[tn]); out->append (1, ';');}
    ::free (strings);
  } else ::backtrace_symbols_fd (array, got, 2);
#else
# warning captureBacktrace: I do not know how to capture backtrace there. Patches welcome.
#endif
}

} // namespace glim

#endif // _GLIM_EXCEPTION_HPP_INCLUDED

/**
 * Special handler for ALL exceptions. Usage:
 * 1) In the `main` module inject this code with:
 *   #define _GLIM_ALL_EXCEPTIONS_CODE
 *   #include <glim/exception.hpp>
 * 2) Link with "-ldl" (for `dlsym`).
 * 3) Use the ExceptionHandler to enable special behaviour in the current thread:
 *   glim::ExceptionHandler traceExceptions (glim::Exception::Options::HANDLE_ALL, glim::captureBacktrace, nullptr);
 *
 * About handing all exceptions see:
 *   http://stackoverflow.com/a/11674810/257568
 *   http://blog.sjinks.pro/c-cpp/969-track-uncaught-exceptions/
 */
#ifdef _GLIM_ALL_EXCEPTIONS_CODE

#include <dlfcn.h> // dlsym

typedef void(*cxa_throw_type)(void*, void*, void(*)(void*)); // Tested with GCC 4.7.
static cxa_throw_type NATIVE_CXA_THROW = 0;

extern "C" void __cxa_throw (void* thrown_exception, void* tinfo, void (*dest)(void*)) {
  if (!NATIVE_CXA_THROW) NATIVE_CXA_THROW = reinterpret_cast<cxa_throw_type> (::dlsym (RTLD_NEXT, "__cxa_throw"));
  if (!NATIVE_CXA_THROW) ::std::terminate();

  using namespace glim;
  uint32_t options = Exception::options();
  if (options & Exception::RENDEZVOUS) Exception::rendezvous();
  if (options & Exception::HANDLE_ALL) {
    exception_handler_fn handler = *Exception::handler();
    if (handler) handler (*Exception::handlerArg());
  }

  NATIVE_CXA_THROW (thrown_exception, tinfo, dest);
}

#undef _GLIM_ALL_EXCEPTIONS_CODE
#endif // _GLIM_ALL_EXCEPTIONS_CODE
