/** \file
 * ucontext-based coroutine library designed to emulate a normal control flow around callbacks. */

// http://en.wikipedia.org/wiki/Setcontext; man 3 makecontext; man 2 getcontext
// http://www.boost.org/doc/libs/1_53_0/libs/context/doc/html/index.html
// g++ -std=c++11 -O1 -Wall -g test_cbcoro.cc -pthread && ./a.out

// NB: There is now a coroutine support in Boost ASIO which can be used to make asynchronous APIs look synchronous in a similar way:
//     https://svn.boost.org/trac/boost/changeset/84311

#include <ucontext.h>
#include <sys/mman.h>  // mmap
#include <string.h>  // strerror
#include <mutex>
#include <atomic>
#include <valgrind/valgrind.h>
#include <glim/exception.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/slist.hpp>

namespace glim {

/// Simplifies turning callback control flows into normal imperative control flows.
class CBCoro {
 public:
  /// "Holds" the CBCoro and will delete it when it is no longer used.
  struct CBCoroPtr {
    CBCoro* _coro;
    CBCoroPtr (CBCoro* coro): _coro (coro) {
      _coro->_users++;
    }
    ~CBCoroPtr() {
      if (--_coro->_users <= 0 && _coro->_delete) delete _coro;
    }
    CBCoro* operator ->() const {return _coro;}
  };

  static constexpr size_t defaultStackSize() {return 512 * 1024;}
  static constexpr uint8_t defaultCacheSize() {return 2;}
 protected:
  typedef boost::container::flat_map<size_t, boost::container::slist<void*> > cache_t;
  /// The cached stacks; stackSize -> free list.
  static cache_t& cache() {static cache_t CACHE; return CACHE;}
  static std::mutex& cacheMutex() {static std::mutex CACHE_MUTEX; return CACHE_MUTEX;}

  ucontext_t _context;
  ucontext_t* _returnTo;
  std::recursive_mutex _mutex;  ///< This one is locked most of the time.
  std::atomic_int_fast32_t _users;  ///< Counter used by `CBCoroPtr`.
  bool _delete;  ///< Whether the `CBCoroPtr` should `delete` this instance when it is no longer used (default is `true`).
  bool _invokeFromYield;  ///< True if `invokeFromCallback()` was called directly from `yieldForCallback()`.
  bool _yieldFromInvoke;  ///< True if `yieldForCallback()` now runs from `invokeFromCallback()`.
  uint8_t const _cacheStack;  ///< Tells `freeStack()` to cache the stack if the number of cached `#_stackSize` stacks is less than it.
  void* _stack;
  size_t const _stackSize;  ///< Keeps the size of the stack.

  /// Peek a stack from the cache or allocate one with `mmap` (and register with Valgrind).
  virtual void allocateStack() {
    if (_cacheStack) {
      std::lock_guard<std::mutex> lock (cacheMutex());
      auto& freeList = cache()[_stackSize];
      if (!freeList.empty()) {_stack = freeList.front(); freeList.pop_front(); return;}
    }
    _stack = mmap (nullptr, _stackSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_NORESERVE, -1, 0);
    if (_stack == MAP_FAILED) GTHROW (std::string ("mmap allocation failed: ") + ::strerror (errno));
    #pragma GCC diagnostic ignored "-Wunused-value"
    VALGRIND_STACK_REGISTER (_stack, (char*) _stack + _stackSize);
  }
  /// Release a stack into the cache or free it with `munmap` (and deregister with Valgrind).
  virtual void freeStack() {
    if (_cacheStack) {
      std::lock_guard<std::mutex> lock (cacheMutex());
      auto& freeList = cache()[_stackSize];
      if (freeList.size() < _cacheStack) {freeList.push_front (_stack); _stack = nullptr; return;}
    }
    VALGRIND_STACK_DEREGISTER (_stack);
    if (munmap (_stack, _stackSize)) GTHROW (std::string ("!munmap: ") + ::strerror (errno));;
    _stack = nullptr;
  }

  /// Prepare the coroutine (initialize context, allocate stack and register it with Valgrind).
  CBCoro (uint8_t cacheStack = defaultCacheSize(), size_t stackSize = defaultStackSize()):
      _returnTo (nullptr), _users (0), _delete (true), _invokeFromYield (false), _yieldFromInvoke (false),
      _cacheStack (cacheStack), _stack (nullptr), _stackSize (stackSize) {
    if (getcontext (&_context)) GTHROW ("!getcontext");
    allocateStack();
    _context.uc_stack.ss_sp = _stack;
    _context.uc_stack.ss_size = stackSize;
  }
  virtual ~CBCoro() {
    freeStack();
  }
 public:
  /// Starts the coroutine on the `_stack` (makecontext, swapcontext), calling the `CBCoro::run`.
  CBCoroPtr start() {
    CBCoroPtr ptr (this);
    ucontext_t back; _context.uc_link = &back;
    makecontext (&_context, (void(*)()) cbcRun, 1, (intptr_t) this);
    // Since we have to "return" from inside the `yieldForCallback`,
    // we're not actually using the `_context.uc_link` and `return`, we use `setcontext (_returnTo)` instead.
    _returnTo = &back;
    _mutex.lock();
    swapcontext (&back, &_context); // Now our stack lives and the caller stack is no longer in control.
    _mutex.unlock();
    return ptr;
  }
 protected:
  /// Logs exception thrown from `CBCoro::run`.
  virtual void log (const std::exception& ex) {
    std::cerr << "glim::CBCoro, exception: " << ex.what() << std::endl;
  }
  static void cbcRun (CBCoro* cbCoro) {
    try {
      cbCoro->run();
    } catch (const std::exception& ex) {
      cbCoro->log (ex);
    }
    cbCoro->cbcReturn();  // Return the control to the rightful owner, e.g. to a last callback who ran `invokeFromCallback`, or otherwise to `cbcStart`.
  }
  /// Relinquish the control to the original owner of the thread, restoring its stack.
  void cbcReturn() {
    ucontext_t* returnTo = _returnTo;
    if (returnTo != nullptr) {_returnTo = nullptr; setcontext (returnTo);}
  }
  /// This method is performed on the CBCoro stack, allowing it to be suspended and then reanimated from callbacks.
  virtual void run() = 0;
 public:
  /** Use this method to wrap a return-via-callback code.
   * For example, the callback code \code
   *   startSomeWork ([=]() {
   *     continueWhenWorkIsFinished();
   *   });
   * \endcode should be turned into \code
   *   yieldForCallback ([&]() {
   *     startSomeWork ([&]() {
   *       invokeFromCallback();
   *     });
   *   });
   *   continueWhenWorkIsFinished();
   * \endcode
   *
   * Captures the stack, runs the `fun` and relinquish the control to `_returnTo`.\n
   * This method will never "return" by itself, in order for it to "return" the
   * `fun` MUST call `invokeFromCallback`, maybe later and from a different stack. */
  template <typename F> CBCoroPtr yieldForCallback (F fun) {
    CBCoroPtr ptr (this);
    _yieldFromInvoke = false;
    if (getcontext (&_context)) GTHROW ("!getcontext");  // Capture.
    if (_yieldFromInvoke) {
      // We're now in the future, revived by the `invokeFromCallback`.
      // All we do now is "return" to the caller whose stack we captured earlier.
    } else {
      // We're still in the present, still have some work to do.
      fun();  // The `fun` is supposed to do something resulting in the `invokeFromCallback` being called later.
      if (_invokeFromYield) {
        // The `fun` used the `invokeFromCallback` directly, not resorting to callbacks, meaning we don't have to do our magick.
        _invokeFromYield = false;
      } else {
        // So, the `fun` took measures to revive us later, it's time for us to go into torpor and return the control to whoever we've borrowed it from.
        cbcReturn();
      }
    }
    return ptr;
  }

  /// To be called from a callback in order to lend the control to CBCoro, continuing it from where it called `yieldForCallback`.
  CBCoroPtr invokeFromCallback() {
    CBCoroPtr ptr (this);
    _mutex.lock();  // Wait for an other-thready `yieldForCallback` to finish.
    if (_returnTo != nullptr) {
      // We have not yet "returned" from the `yieldForCallback`,
      // meaning that the `invokeFromCallback` was executed immediately from inside the `yieldForCallback`.
      // In that case we must DO NOTHING, we must simply continue running on the current stack.
      _invokeFromYield = true;  // Tells `yieldForCallback` to do nothing.
    } else {
      // Revive the CBCoro, letting it continue from where it was suspended in `yieldForCallback`.
      ucontext_t cbContext; _returnTo = &cbContext; _yieldFromInvoke = true;
      if (swapcontext (&cbContext, &_context)) GTHROW ("!swapcontext");
      // NB: When the CBCoro is suspended or exits, the control returns back there and then back to the callback from which we borrowed it.
      if (_returnTo == &cbContext) _returnTo = nullptr;
    }
    _mutex.unlock();  // Other-thready `yieldForCallback` has finished and `cbcReturn`ed here.
    return ptr;
  }
};

/** CBCoro running a given functor.
 * The functor's first argument must be a CBCoro pointer, like this: \code (new CBCoroForFunctor ([](CBCoro* cbcoro) {}))->start(); \endcode */
template <typename FUN> struct CBCoroForFunctor: public CBCoro {
  FUN _fun;
  template <typename CFUN> CBCoroForFunctor (CFUN&& fun, uint8_t cacheStack, size_t stackSize): CBCoro (cacheStack, stackSize), _fun (std::forward<CFUN> (fun)) {}
  virtual void run() {_fun (this);}
  virtual ~CBCoroForFunctor() {}
};

/** Syntactic sugar: Runs a given functor in a CBCoro instance.
 * Example: \code glim::cbCoro ([](glim::CBCoro* cbcoro) {}); \endcode
 * Returns a `CBCoroPtr` to the CBCoro instance holding the `fun` which might be held somewhere in order to delay the deletion of `fun`. */
template <typename FUN> inline CBCoro::CBCoroPtr cbCoro (FUN&& fun, uint8_t cacheStack = CBCoro::defaultCacheSize(), size_t stackSize = CBCoro::defaultStackSize()) {
  return (new CBCoroForFunctor<FUN> (std::forward<FUN> (fun), cacheStack, stackSize))->start();
}

}
