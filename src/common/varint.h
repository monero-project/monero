// Copyright (c) 2014-2020, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <limits>
#include <type_traits>
#include <utility>
#include <sstream>
#include <string>
/*! \file varint.h
 * \brief provides the implementation of varint's
 * 
 * The representation of varints is rather odd. The first bit of each
 * octet is significant, it represents wheter there is another part
 * waiting to be read. For example 0x8002 would return 0x200, even
 * though 0x02 does not have its msb set. The actual way they are read
 * is as follows: Strip the msb of each byte, then from left to right,
 * read in what remains, placing it in reverse, into the buffer. Thus,
 * the following bit stream: 0xff02 would return 0x027f. 0xff turns
 * into 0x7f, is placed on the beginning of the buffer, then 0x02 is
 * unchanged, since its msb is not set, and placed at the end of the
 * buffer.
 */

namespace tools {

  /*! \brief Error codes for varint
   */
  enum {
    /* \brief Represents the overflow error */
    EVARINT_OVERFLOW = -1,
    /* \brief Represents a non conical represnetation */
    EVARINT_REPRESENT = -2,
  };

  /*! \brief writes a varint to a stream.
   */
  template<typename OutputIt, typename T>
  /* Requires T to be both an integral type and unsigned, should be a compile error if it is not */
  typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, void>::type 
  write_varint(OutputIt &&dest, T i) {
    /* Make sure that there is one after this */
    while (i >= 0x80) {
      *dest = (static_cast<char>(i) & 0x7f) | 0x80; 
      ++dest;
      i >>= 7;			/* I should be in multiples of 7, this should just get the next part */
    }
    /* writes the last one to dest */
    *dest = static_cast<char>(i);
    dest++;			/* Seems kinda pointless... */
  }

  /*! \brief Returns the string that represents the varint
   */
  template<typename T>
    std::string get_varint_data(const T& v)
    {
      std::stringstream ss;
      write_varint(std::ostreambuf_iterator<char>(ss), v);
      return ss.str();
    }
  /*! \brief reads in the varint that is pointed to by InputIt into write
   */ 
  template<int bits, typename InputIt, typename T>
    typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value && 0 <= bits && bits <= std::numeric_limits<T>::digits, int>::type
    read_varint(InputIt &&first, InputIt &&last, T &write) {
    int read = 0;
    write = 0;
    for (int shift = 0;; shift += 7) {
      if (first == last) {
	return read; 
      }
      unsigned char byte = *first;
      ++first;
      ++read;
      if (shift + 7 >= bits && byte >= 1 << (bits - shift)) {
	return EVARINT_OVERFLOW;
      }
      if (byte == 0 && shift != 0) {
	return EVARINT_REPRESENT;
      }

      write |= static_cast<T>(byte & 0x7f) << shift; /* Does the actually placing into write, stripping the first bit */

      /* If there is no next */
      if ((byte & 0x80) == 0) {
	break;
      }
    }
    return read;
  }

  /*! \brief Wrapper around the other read_varint,
   *  Sets template parameters for you.
   */
  template<typename InputIt, typename T>
    int read_varint(InputIt &&first, InputIt &&last, T &i) {
    return read_varint<std::numeric_limits<T>::digits>(std::forward<InputIt>(first), std::forward<InputIt>(last), i);
  }
}
