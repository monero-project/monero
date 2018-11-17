// Copyright (c) 2019, The Monero Project
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
#include <random>
#include <vector>

namespace tools
{
    //! Select outputs using a gamma distribution with Sarang's output-lineup method
    class gamma_picker
    {
        std::vector<uint64_t> rct_offsets;
        std::gamma_distribution<double> gamma;
        double outputs_per_second;

        gamma_picker(const gamma_picker&) = default; // force explicit usage of `clone()` to copy.
    public:
        //! \post `!is_valid()` since the chain of offsets is empty.
        gamma_picker()
          : gamma_picker(std::vector<std::uint64_t>{})
        {}

        //! Use default (recommended) gamma parameters with `rct_offsets`.
        explicit gamma_picker(std::vector<std::uint64_t> rct_offsets);
        explicit gamma_picker(std::vector<std::uint64_t> rct_offsets, double shape, double scale);

        //! \post Source of move `!is_valid()`.
        gamma_picker(gamma_picker&&) = default;

        //! \post Source of move `!is_valid()`.
        gamma_picker& operator=(gamma_picker&&) = default;

        //! \return A copy of `this`.
        gamma_picker clone() const { return gamma_picker{*this}; }

        //! \return `is_valid()`.
        explicit operator bool() const noexcept { return is_valid(); }

        //! \return True if `operator()()` can pick an output using `offsets()`.
        bool is_valid() const noexcept;

        //! \return An upper-bound on the number of unlocked/spendable outputs based on block age.
        std::uint64_t spendable_upper_bound() const noexcept;

        /*!
            Select a random output index for use in a ring. Outputs in the
            unspendable range (too new) and older than the chain (too old) are
            filtered out by retrying the gamma distribution.

            \throw std::logic_error if `!is_valid()` - considered unrecoverable.
            \throw std::runtiime_error if no output within spendable range was
                selected after 100 attempts.
            \return Selected output using gamma distribution.
        */
        std::uint64_t operator()();

        //! \return Current ringct distribution used for `operator()()` output selection.
        const std::vector<std::uint64_t>& offsets() const noexcept { return rct_offsets; }

        //! \return Ownership of `offsets()` by move. \post `!is_valid()`
        std::vector<std::uint64_t> take_offsets();
    };
}
