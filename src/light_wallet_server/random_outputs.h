// Copyright (c) 2018-2019, The Monero Project
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

#include <cstdint>
#include <functional>
#include <vector>

#include "common/expect.h"
#include "rpc/message_data_structs.h"
#include "span.h"
#include "wallet/fwd.h"

namespace lws
{
    using histogram = cryptonote::rpc::output_amount_count;
    using output_ref = cryptonote::rpc::output_amount_and_index;
    using output_keys = cryptonote::rpc::output_key_mask_unlocked;

    struct random_output
    {
        output_keys keys;
        std::uint64_t index;
    };

    struct random_ring
    {
        std::vector<random_output> ring;
        std::uint64_t amount;
    };

    using key_fetcher = expect<std::vector<output_keys>>(std::vector<output_ref>);

    /*!
        Selects random outputs for use in a ring signature. `amounts` of `0`
        use a gamma distribution algorithm and all other amounts use a
        triangular distribution.

        \param mixin The number of dummy outputs per ring.
        \param amounts The amounts that need dummy outputs to be selected.
        \param pick_rct Ring-ct distribution from the daemon
        \param histograms A histogram from the daemon foreach non-zero value
            in `amounts`.
        \param fetch A function that can retrieve the keys for the randomly
            selected outputs.

        \note `histograms` is modified - the list is sorted by amount.

        \note This currenty leaks the real outputs to `fetch`, because the
            real output is not provided alongside the dummy outputs. This is a
            limitation of the current openmonero/mymonero API. When this is
            resolved, this function can possibly be moved outside of the `lws`
            namespace for use by simple wallet.

        \return Randomly selected outputs in rings of size `mixin`, one for
            each element in `amounts`. Amounts with less than `mixin` available
            are not returned. All outputs are unlocked.
    */
    expect<std::vector<random_ring>> pick_random_outputs(
        std::uint32_t mixin,
        epee::span<const std::uint64_t> amounts,
        tools::gamma_picker& pick_rct,
        epee::span<histogram> histograms,
        std::function<key_fetcher> fetch
    );
}

