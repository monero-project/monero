#include <stddef.h>

#if defined(__APPLE__)
#include <mach-o/getsect.h>

#if !defined(__LP64__)
extern const struct mach_header _mh_execute_header;
#else
extern const struct mach_header_64 _mh_execute_header;
#endif

const unsigned char *get_blocks_dat_start()
{
    size_t size;
    return getsectiondata(&_mh_execute_header, "__DATA", "__blocks_dat", &size);
}

size_t get_blocks_dat_size()
{
    size_t size;
    getsectiondata(&_mh_execute_header, "__DATA", "__blocks_dat", &size);
    return size;
}

#else

#if defined(_WIN32) && !defined(_WIN64)
#define _binary_blocks_start binary_blocks_dat_start
#define _binary_blocks_end binary_blocks_dat_end
#else
#define _binary_blocks_start _binary_blocks_dat_start
#define _binary_blocks_end _binary_blocks_dat_end
#endif

extern const unsigned char _binary_blocks_start[];
extern const unsigned char _binary_blocks_end[];

const unsigned char *get_blocks_dat_start(void)
{
	return _binary_blocks_start;
}

size_t get_blocks_dat_size(void)
{
	return (size_t) (_binary_blocks_end - _binary_blocks_start);
}

#endif
