// Copyright (c) 2018, The Monero Project
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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "crypto/crypto.h"
#include "light_wallet_server/fwd.h"
#include "light_wallet_server/db/fwd.h"

namespace lws
{
    //! Tracks a subset of DB account info for scanning/updating.
    class account
    {
        struct internal;
        
        std::shared_ptr<const internal> immutable_;
        std::vector<db::output_id> spendable_;
        std::vector<crypto::public_key> pubs_;
        std::vector<db::spend> spends_;
        std::vector<db::output> outputs_;
        db::block_id height_;

        explicit account(std::shared_ptr<const internal> immutable, db::block_id height, std::vector<db::output_id> spendable, std::vector<crypto::public_key> pubs) noexcept;
        void null_check() const;

    public:

        //! Construct an account from `source` and current `spendable` outputs.
        explicit account(db::account const& source, std::vector<db::output_id> spendable, std::vector<crypto::public_key> pubs);

        /*!
            \return False if this is a "moved-from" account (i.e. the internal
                memory has been moved to another object).
        */
        explicit operator bool() const noexcept { return immutable_ != nullptr; }

        account(const account&) = delete;
        account(account&&) = default;
        ~account() noexcept;
        account& operator=(const account&) = delete;
        account& operator=(account&&) = default;

        //! \return A copy of `this`.
        account clone() const;

        //! \return A copy of `this` with a new height and `outputs().empty()`.
        void updated(db::block_id new_height) noexcept;

        //! \return Unique ID from the account database, possibly `db::account_id::kInvalid`.
        db::account_id id() const noexcept;

        //! \return Monero base58 string for account.
        std::string const& address() const;

        //! \return Object used for lookup in LMDB.
        db::account_address const& db_address() const;

        //! \return Extracted view public key from `address()`
        crypto::public_key const& view_public() const;

        //! \return Extracted spend public key from `address()`.
        crypto::public_key const& spend_public() const;

        //! \return Secret view key for the account.
        crypto::secret_key const& view_key() const;

        //! \return Current scan height of `this`.
        db::block_id scan_height() const noexcept { return height_; }

        //! \return True iff `id` is spendable by `this`.
        bool has_spendable(db::output_id const& id) const noexcept;

        //! \return Outputs matched during the latest scan.
        std::vector<db::output> const& outputs() const noexcept { return outputs_; }

        //! \return Spends matched during the latest scan.
        std::vector<db::spend> const& spends() const noexcept { return spends_; }

        //! Track a newly received `out`, \return `false` if `out.pub` is duplicated.
        bool add_out(db::output const& out);

        //! Track a possible `spend`.
        void add_spend(db::spend const& spend);
    };
} // lws
