#ifndef _TSC_TIMER_H
#define _TSC_TIMER_H

namespace glim {

extern "C" {  // http://en.wikipedia.org/wiki/Rdtsc
#if (defined(__GNUC__) || defined(__ICC)) && defined(__i386__)
 static __inline__ unsigned long long rdTsc(void) {
  unsigned long long ret;
  __asm__ __volatile__("rdtsc": "=A" (ret));
  return ret;
 }
#elif (defined(__GNUC__) || defined(__ICC) || defined(__SUNPRO_C)) && defined(__x86_64__)
 static __inline__ unsigned long long rdTsc(void) {
  unsigned a, d;
  asm volatile("rdtsc" : "=a" (a), "=d" (d));
  return ((unsigned long long)a) | (((unsigned long long)d) << 32);
 }
#endif
}

//! CPU cycles timer. Fast, not safe.
//! cf. http://en.wikipedia.org/wiki/Rdtsc
struct TscTimer {
  int64_t start;
  TscTimer (): start (rdTsc()) {}
  int64_t operator()() const {return rdTsc() - start;}
};

}

#endif // _TSC_TIMER_H
