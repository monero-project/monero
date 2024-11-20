// Copyright (c) 2024, The Monero Project
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

#include <algorithm>
#include <vector>

namespace tools
{

template<typename T, typename U>
bool merge_sorted_vectors(const std::vector<T> &a, const std::vector<T> &b, const U &sort_fn, std::vector<T> &v_out)
{
    v_out.clear();
    v_out.reserve(a.size() + b.size());

    if (!std::is_sorted(a.begin(), a.end(), sort_fn))
        return false;
    if (!std::is_sorted(b.begin(), b.end(), sort_fn))
        return false;

    auto a_it = a.begin();
    auto b_it = b.begin();

    while (a_it != a.end() || b_it != b.end())
    {
        if (a_it == a.end())
        {
            v_out.push_back(*b_it);
            ++b_it;
            continue;
        }

        if (b_it == b.end())
        {
            v_out.push_back(*a_it);
            ++a_it;
            continue;
        }

        if (sort_fn(*a_it, *b_it))
        {
            v_out.push_back(*a_it);
            ++a_it;
            continue;
        }

        v_out.push_back(*b_it);
        ++b_it;
    }

    assert(std::is_sorted(v_out.begin(), v_out.end(), sort_fn));
    return true;
}

}//namespace tools
