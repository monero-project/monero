#ifndef PORTABLE_BINARY_ARCHIVE_HPP
#define PORTABLE_BINARY_ARCHIVE_HPP

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <boost/archive/archive_exception.hpp>

#include <climits>
#if CHAR_BIT != 8
#error This code assumes an eight-bit byte.
#endif

#include <boost/archive/basic_archive.hpp>
#include <boost/predef/other/endian.h>

#include <boost/archive/impl/archive_serializer_map.ipp>

namespace boost { namespace archive {

enum portable_binary_archive_flags {
    endian_big        = 0x4000,
    endian_little     = 0x8000
};

//#if ( endian_big <= boost::archive::flags_last )
//#error archive flags conflict
//#endif

inline void
reverse_bytes(signed char size, char *address){
    if (size <= 0)
        throw archive_exception(archive_exception::other_exception);
    char * first = address;
    char * last = first + size - 1;
    for(;first < last;++first, --last){
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow="
#endif
        char x = *last;
        *last = *first;
        *first = x;
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    }
}

} }

#endif // PORTABLE_BINARY_ARCHIVE_HPP
