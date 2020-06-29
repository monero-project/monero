#include <cstddef>
#include <cstdint>
#include <strings.h>
#include <string.h>
#include <glob.h>
#include <unistd.h>
#include <fnmatch.h>

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

// Prior to GLIBC_2.14, memcpy was aliased to memmove.
extern "C" void* memmove(void* a, const void* b, size_t c);
//extern "C" void* memset(void* a, int b, long unsigned int c);
extern "C" void* memcpy(void* a, const void* b, size_t c)
{
    return memmove(a, b, c);
}

extern "C" void __chk_fail(void) __attribute__((__noreturn__));

#if defined(__i386__) || defined(__arm__)

extern "C" int64_t __udivmoddi4(uint64_t u, uint64_t v, uint64_t* rp);

extern "C" int64_t __wrap___divmoddi4(int64_t u, int64_t v, int64_t* rp)
{
    int32_t c1 = 0, c2 = 0;
    int64_t uu = u, vv = v;
    int64_t w;
    int64_t r;

    if (uu < 0) {
        c1 = ~c1, c2 = ~c2, uu = -uu;
    }
    if (vv < 0) {
        c1 = ~c1, vv = -vv;
    }

    w = __udivmoddi4(uu, vv, (uint64_t*)&r);
    if (c1)
        w = -w;
    if (c2)
        r = -r;

    *rp = r;
    return w;
}
#endif

/* glibc-internal users use __explicit_bzero_chk, and explicit_bzero
   redirects to that.  */
#undef explicit_bzero
/* Set LEN bytes of S to 0.  The compiler will not delete a call to
   this function, even if S is dead after the call.  */
void
explicit_bzero (void *s, size_t len)
{
  memset (s, '\0', len);
  /* Compiler barrier.  */
  asm volatile ("" ::: "memory");
}

// Redefine explicit_bzero_chk
void
__explicit_bzero_chk (void *dst, size_t len, size_t dstlen)
{
  /* Inline __memset_chk to avoid a PLT reference to __memset_chk.  */
  if (__glibc_unlikely (dstlen < len))
    __chk_fail ();
  memset (dst, '\0', len);
  /* Compiler barrier.  */
  asm volatile ("" ::: "memory");
}
/* libc-internal references use the hidden
   __explicit_bzero_chk_internal symbol.  This is necessary if
   __explicit_bzero_chk is implemented as an IFUNC because some
   targets do not support hidden references to IFUNC symbols.  */
#define strong_alias (__explicit_bzero_chk, __explicit_bzero_chk_internal)

#undef glob
extern "C" int glob_old(const char * pattern, int flags, int (*errfunc) (const char *epath, int eerrno), glob_t *pglob);
#ifdef __i386__
__asm__(".symver glob_old,glob@GLIBC_2.0");
#elif defined(__amd64__)
__asm__(".symver glob_old,glob@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver glob_old,glob@GLIBC_2.4");
#elif defined(__aarch64__)
__asm__(".symver glob_old,glob@GLIBC_2.17");
#endif

extern "C" int __wrap_glob(const char * pattern, int flags, int (*errfunc) (const char *epath, int eerrno), glob_t *pglob)
{
    return glob_old(pattern, flags, errfunc, pglob);
}

