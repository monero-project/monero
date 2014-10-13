#ifndef _NSEC_TIMER_H
#define _NSEC_TIMER_H

#include <time.h> // clock_gettime, CLOCK_MONOTONIC
#include <stdint.h>
#include <string>
#include <sstream>

namespace glim {

//! Safe nanoseconds timer.
struct NsecTimer {
  timespec start;
  NsecTimer () {restart();}
  //! Nanoseconds since the creation or restart of the timer.
  int64_t operator()() const {
    timespec nsecStop; clock_gettime (CLOCK_MONOTONIC, &nsecStop);
    return (int64_t) (nsecStop.tv_sec - start.tv_sec) * 1000000000LL + (int64_t) (nsecStop.tv_nsec - start.tv_nsec);
  }
  /** Seconds since the creation or restart of the timer. */
  double sec() const {
    timespec nsecStop; clock_gettime (CLOCK_MONOTONIC, &nsecStop);
    double seconds = nsecStop.tv_sec - start.tv_sec;
    seconds += (double)(nsecStop.tv_nsec - start.tv_nsec) / 1000000000.0;
    return seconds;
  }
  //! Seconds since the creation or restart of the timer.
  std::string seconds (int precision = 9) const {
    // The trick is to avoid the scientific notation by printing integers.
    double sec = this->sec();
    std::ostringstream buf;
    int isec = (int) sec;
    buf << isec;

    sec -= isec;
    for (int pc = precision; pc; --pc) sec *= 10.0;
    int ifrac = (int) sec;
    if (ifrac > 0) {
      buf << '.';
      buf.fill ('0'); buf.width (precision);
      buf << ifrac;
    }
    return buf.str();
  }
  void restart() {clock_gettime (CLOCK_MONOTONIC, &start);}
  int64_t getAndRestart() {int64_t tmp = operator()(); restart(); return tmp;}
};

}

#endif // _NSEC_TIMER_H
