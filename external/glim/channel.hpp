#ifndef _GLIM_CHANNEL_INCLUDED
#define _GLIM_CHANNEL_INCLUDED

#include <atomic>
#include <mutex>
#include <thread>

namespace glim {

/// Unbuffered channel.
/// Optimized for a single value (busy-waits on a second one).
template <typename V>
struct Channel {
  V _v;
  std::mutex _mutex;  // Locked when there is no value.
  std::atomic_int_fast8_t _state; enum State {EMPTY = 0, WRITING = 1, FULL = 2};
  Channel(): _state (EMPTY) {_mutex.lock();}
  // Waits until the Channel is empty then stores the value.
  template <typename VA> void send (VA&& v) {
    for (;;) {
      int_fast8_t expectEmpty = EMPTY; if (_state.compare_exchange_weak (expectEmpty, WRITING)) break;
      std::this_thread::sleep_for (std::chrono::milliseconds (20));
    }
    try {_v = std::forward<V> (v);} catch (...) {_state = EMPTY; throw;}
    _state = FULL;
    _mutex.unlock();  // Allows the reader to proceed.
  }
  // Waits untill there is a value to receive.
  V receive() {
    _mutex.lock();  // Wait.
    V tmp = std::move (_v);
    assert (_state == FULL);
    _state = EMPTY;
    return tmp;
  }
};

} // namespace glim

#endif
