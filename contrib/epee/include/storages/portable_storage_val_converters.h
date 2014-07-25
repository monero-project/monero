// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//



#pragma once

#include "misc_language.h"
#include "portable_storage_base.h"
#include "warnings.h"

namespace epee
{
  namespace serialization
  {
#define ASSERT_AND_THROW_WRONG_CONVERSION() ASSERT_MES_AND_THROW("WRONG DATA CONVERSION: from type=" << typeid(from).name() << " to type " << typeid(to).name())

    template<typename from_type, typename to_type>
    void convert_int_to_uint(const from_type& from, to_type& to)
    {
PUSH_WARNINGS
DISABLE_VS_WARNINGS(4018)
      CHECK_AND_ASSERT_THROW_MES(from >=0, "unexpected int value with signed storage value less than 0, and unsigned receiver value");
DISABLE_GCC_AND_CLANG_WARNING(sign-compare)
      CHECK_AND_ASSERT_THROW_MES(from <= std::numeric_limits<to_type>::max(), "int value overhead: try to set value " << from << " to type " << typeid(to_type).name() << " with max possible value = " << std::numeric_limits<to_type>::max());
      to = static_cast<to_type>(from);
POP_WARNINGS
    }
    template<typename from_type, typename to_type>
    void convert_int_to_int(const from_type& from, to_type& to)
    {
      CHECK_AND_ASSERT_THROW_MES(from >= boost::numeric::bounds<to_type>::lowest(), "int value overhead: try to set value " << from << " to type " << typeid(to_type).name() << " with lowest possible value = " << boost::numeric::bounds<to_type>::lowest());
PUSH_WARNINGS
DISABLE_CLANG_WARNING(tautological-constant-out-of-range-compare)
      CHECK_AND_ASSERT_THROW_MES(from <= std::numeric_limits<to_type>::max(), "int value overhead: try to set value " << from << " to type " << typeid(to_type).name() << " with max possible value = " << std::numeric_limits<to_type>::max());
POP_WARNINGS
      to = static_cast<to_type>(from);
    }
    template<typename from_type, typename to_type>
    void convert_uint_to_any_int(const from_type& from, to_type& to)
    {
PUSH_WARNINGS
DISABLE_VS_WARNINGS(4018)
DISABLE_CLANG_WARNING(tautological-constant-out-of-range-compare)
        CHECK_AND_ASSERT_THROW_MES(from <= std::numeric_limits<to_type>::max(), "uint value overhead: try to set value " << from << " to type " << typeid(to_type).name() << " with max possible value = " << std::numeric_limits<to_type>::max());
      to = static_cast<to_type>(from);
POP_WARNINGS
    }

    template<typename from_type, typename to_type, bool, bool> //is from signed, is from to signed
    struct convert_to_signed_unsigned;

    template<typename from_type, typename to_type>
    struct convert_to_signed_unsigned<from_type, to_type, true, true>
    {
      static void convert(const from_type& from, to_type& to)
      {//from signed to signed
        convert_int_to_int(from, to);
      }
    };

    template<typename from_type, typename to_type>
    struct convert_to_signed_unsigned<from_type, to_type, true, false>
    {
      static void convert(const from_type& from, to_type& to)
      {//from signed to unsigned
        convert_int_to_uint(from, to);
      }
    };

    template<typename from_type, typename to_type>
    struct convert_to_signed_unsigned<from_type, to_type, false, true>
    {
      static void convert(const from_type& from, to_type& to)
      {//from unsigned to signed
        convert_uint_to_any_int(from, to);
      }
    };

    template<typename from_type, typename to_type>
    struct convert_to_signed_unsigned<from_type, to_type, false, false>
    {
      static void convert(const from_type& from, to_type& to)
      {
        //from unsigned to unsigned
        convert_uint_to_any_int(from, to);
      }
    };

    template<typename from_type, typename to_type, bool>
    struct convert_to_integral;

    template<typename from_type, typename to_type>
    struct convert_to_integral<from_type, to_type, true>
    {
      static void convert(const from_type& from, to_type& to)
      {
        convert_to_signed_unsigned<from_type, to_type, std::is_signed<from_type>::value, std::is_signed<to_type>::value>::convert(from, to);
      }
    };

    template<typename from_type, typename to_type>
    struct convert_to_integral<from_type, to_type, false>
    {
      static void convert(const from_type& from, to_type& to)
      {
        ASSERT_AND_THROW_WRONG_CONVERSION();
      }
    };

    template<class from_type, class to_type>
    struct is_convertable: std::integral_constant<bool,
                            std::is_integral<to_type>::value &&
                            std::is_integral<from_type>::value &&
                            !std::is_same<from_type, bool>::value &&
                            !std::is_same<to_type, bool>::value > {};

    template<typename from_type, typename to_type, bool>
    struct convert_to_same;

    template<typename from_type, typename to_type>
    struct convert_to_same<from_type, to_type, true>
    {
      static void convert(const from_type& from, to_type& to)
      {
        to = from;
      }
    };

    template<typename from_type, typename to_type>
    struct convert_to_same<from_type, to_type, false>
    {
      static void convert(const from_type& from, to_type& to)
      {
        convert_to_integral<from_type, to_type, is_convertable<from_type, to_type>::value>::convert(from, to);// ASSERT_AND_THROW_WRONG_CONVERSION();
      }
    };


    template<class from_type, class to_type>
    void convert_t(const from_type& from, to_type& to)
    {
      convert_to_same<from_type, to_type, std::is_same<to_type, from_type>::value>::convert(from, to);
    }
  }
}