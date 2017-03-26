#include <stddef.h>

#if defined(__APPLE__)
#include <mach-o/getsect.h>
#ifdef BUILD_SHARED_LIBS
#if !defined(__LP64__)
const struct mach_header _mh_execute_header;
#else
const struct mach_header_64 _mh_execute_header;
#endif
#else
#if !defined(__LP64__)
extern const struct mach_header _mh_execute_header;
#else
extern const struct mach_header_64 _mh_execute_header;
#endif
#endif

const unsigned char *get_blocks_dat_start(int testnet)
{
    size_t size;
    if (testnet)
        return getsectiondata(&_mh_execute_header, "__DATA", "__testnet_blocks_dat", &size);
    else
        return getsectiondata(&_mh_execute_header, "__DATA", "__blocks_dat", &size);
}

size_t get_blocks_dat_size(int testnet)
{
    size_t size;
    if (testnet)
        getsectiondata(&_mh_execute_header, "__DATA", "__testnet_blocks_dat", &size);
    else
        getsectiondata(&_mh_execute_header, "__DATA", "__blocks_dat", &size);
    return size;
}

#else

#if defined(_WIN32) && !defined(_WIN64)
#define _binary_blocks_start binary_blocks_dat_start
#define _binary_blocks_end binary_blocks_dat_end
#define _binary_testnet_blocks_start binary_testnet_blocks_dat_start
#define _binary_testnet_blocks_end binary_testnet_blocks_dat_end
#else
#define _binary_blocks_start _binary_blocks_dat_start
#define _binary_blocks_end _binary_blocks_dat_end
#define _binary_testnet_blocks_start _binary_testnet_blocks_dat_start
#define _binary_testnet_blocks_end _binary_testnet_blocks_dat_end
#endif

extern const unsigned char _binary_blocks_start[];
extern const unsigned char _binary_blocks_end[];
extern const unsigned char _binary_testnet_blocks_start[];
extern const unsigned char _binary_testnet_blocks_end[];

const unsigned char *get_blocks_dat_start(int testnet)
{
  if (testnet)
    return _binary_testnet_blocks_start;
  else
    return _binary_blocks_start;
}

size_t get_blocks_dat_size(int testnet)
{
  if (testnet)
    return (size_t) (_binary_testnet_blocks_end - _binary_testnet_blocks_start);
  else
    return (size_t) (_binary_blocks_end - _binary_blocks_start);
}

#endif
