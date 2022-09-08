// Copyright (c) 2022, The Monero Project
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

#pragma once

#include <cstddef>
#include <cstdint>

#include "../internal/enable_if.h"

#define BLOBBABLE_BASE(basename) using blobbable_base_type = basename;

#define IS_POD(ty) std::is_pod<ty>::value
#define IS_BLOBBABLE(ty) IS_POD(ty) || serde::internal::has_blobbable_base<ty>::value
#define BLOB_TYPE(ty) typename serde::internal::blobbable<ty>::blob_type

namespace serde::internal
{
    /*
     * @brief checks for the existence of member type blobbable_base_type in type U
     */
    template <typename U>
    class has_blobbable_base
    {
    private:
        template<typename T, typename = typename T::blobbable_base_type>
        static std::uint8_t check(int);
        template<typename T> static std::uint16_t check(...);

    public:
        static
        constexpr bool value = sizeof(check<U>(0)) == sizeof(std::uint8_t);
    };

    /*
     * @brief creates reference type to a base of U which is POD and shares some properties with U
     */
    template <typename U, ENABLE_TPARAM_IF(IS_BLOBBABLE(U))>
    class blobbable
    {
    private:
        // This gets instantiated if U is a POD type. return type = U
        template <typename T, ENABLE_TPARAM_IF(IS_POD(T))>
        static T& blobty();
        
        // This gets instantiated if U has member type blobbable_base_type and
        // sizeof(blobbable_base_type) == sizeof(U). return type = blobbable_base_type
        template <typename T, typename B = typename T::blobbable_base_type>
        static BLOB_TYPE(B)& blobty();

    public:

        // We add a reference to the return type then remove it here since functions cannot return
        // arrays, but they can return references to arrays.
        using blob_type = typename std::remove_reference<decltype(blobty<U>())>::type;
        using ref_type = blob_type&;
        using const_ref_type = const blob_type&;
        using ptr_type = blob_type*;
        using const_ptr_type = const blob_type*;
    };
}
