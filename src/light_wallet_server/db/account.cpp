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
#include "account.h"

#include <algorithm>
#include <cstring>

#include "common/error.h"
#include "common/expect.h"
#include "light_wallet_server/db/data.h"
#include "light_wallet_server/db/string.h"

namespace lws
{
    namespace
    {
        // update if `crypto::public_key` gets `operator<`
        struct sort_pubs
        {
            bool operator()(crypto::public_key const& lhs, crypto::public_key const& rhs) const noexcept
            {
                return std::memcmp(std::addressof(lhs), std::addressof(rhs), sizeof(lhs)) < 0;
            }
        };
    }
    struct account::internal
    {
        explicit internal(db::account const& source)
          : address(db::address_string(source.address)), id(source.id), pubs(source.address), view_key()
        {
            using inner_type =
                std::remove_reference<decltype(tools::unwrap(view_key))>::type;

            static_assert(
                std::is_standard_layout<db::view_key>(), "need standard layout source"
            );
            static_assert(std::is_pod<inner_type>(), "need pod target");
            static_assert(sizeof(view_key) == sizeof(source.key), "different size keys");
            std::memcpy(
                std::addressof(tools::unwrap(view_key)),
                std::addressof(source.key),
                sizeof(source.key)
            );
        }

        std::string address;
        db::account_id id;
        db::account_address pubs;
        crypto::secret_key view_key;
    };

    account::account(std::shared_ptr<const internal> immutable, db::block_id height, std::vector<db::output_id> spendable, std::vector<crypto::public_key> pubs) noexcept
      : immutable_(std::move(immutable))
      , spendable_(std::move(spendable))
      , pubs_(std::move(pubs))
      , spends_()
      , outputs_()
      , height_(height)
    {}

    void account::null_check() const
    {
        if (!immutable_)
            MONERO_THROW(::common_error::kInvalidArgument, "using moved from account");
    }

    account::account(db::account const& source, std::vector<db::output_id> spendable, std::vector<crypto::public_key> pubs)
      : account(std::make_shared<internal>(source), source.scan_height, std::move(spendable), std::move(pubs))
    {
        std::sort(spendable_.begin(), spendable_.end());
        std::sort(pubs_.begin(), pubs_.end(), sort_pubs{});
    }

    account::~account() noexcept
    {}

    account account::clone() const
    {
        account result{immutable_, height_, spendable_, pubs_};
        result.outputs_ = outputs_;
        result.spends_ = spends_;
        return result;
    }

    void account::updated(db::block_id new_height) noexcept
    {
        height_ = new_height;
        spends_.clear();
        spends_.shrink_to_fit();
        outputs_.clear();
        outputs_.shrink_to_fit();
    }

    db::account_id account::id() const noexcept
    {
        if (immutable_)
            return immutable_->id;
        return db::account_id::invalid;
    }

    std::string const& account::address() const
    {
        null_check();
        return immutable_->address;
    }

    db::account_address const& account::db_address() const
    {
        null_check();
        return immutable_->pubs;
    }

    crypto::public_key const& account::view_public() const
    {
        null_check();
        return immutable_->pubs.view_public;
    }

    crypto::public_key const& account::spend_public() const
    {
        null_check();
        return immutable_->pubs.spend_public;
    }

    crypto::secret_key const& account::view_key() const
    {
        null_check();
        return immutable_->view_key;
    }

    bool account::has_spendable(db::output_id const& id) const noexcept
    {
        return std::binary_search(spendable_.begin(), spendable_.end(), id);
    }

    bool account::add_out(db::output const& out)
    {
        auto existing_pub = std::lower_bound(pubs_.begin(), pubs_.end(), out.pub, sort_pubs{});
        if (existing_pub != pubs_.end() && *existing_pub == out.pub)
            return false;

        pubs_.insert(existing_pub, out.pub);
        spendable_.insert(
            std::lower_bound(spendable_.begin(), spendable_.end(), out.spend_meta.id),
            out.spend_meta.id
        );
        outputs_.push_back(out);
        return true;
    }

    void account::add_spend(db::spend const& spend)
    {
        spends_.push_back(spend);
    }
} // lws

