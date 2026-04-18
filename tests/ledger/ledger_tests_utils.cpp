// Copyright (c) 2026, The Monero Project
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

//paired header
#include "ledger_tests_utils.h"

//local headers
#include "crypto/generators.h"
#include "cryptonote_basic/account.h"
#include "device/device.hpp"
#include "device/device_io_tcp.hpp"
#include "device/device_ledger.hpp"
#include "net/net_parse_helpers.h"
#include "ringct/rctOps.h"
#include "ringct/rctTypes.h"

//third party headers
#include <boost/asio/io_context.hpp>

//standard headers
#include <memory>
#include <string>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "mock"

namespace
{
boost::asio::io_context global_io_context;
std::optional<hw::ledger::device_ledger> global_ledger_device;
std::optional<cryptonote::account_base> global_ledger_account;
} //anonymous namespace

namespace mock
{
//----------------------------------------------------------------------------------------------------------------------
void init_global_ledger_account(const bool speculos, const std::string &speculos_address)
{
    assert(speculos_address.empty() || speculos);
    if (speculos)
    {
        std::string speculos_host = hw::io::device_io_tcp::DEFAULT_ADDRESS;
        unsigned int speculos_port = hw::io::device_io_tcp::DEFAULT_PORT;
        if (speculos_address.size())
        {
            epee::net_utils::http::url_content url_content;
            if (!epee::net_utils::parse_url(speculos_address, url_content))
                throw std::runtime_error("Could not parse Speculos URL: " + speculos_address);
            speculos_host = url_content.host;
            if (url_content.port)
                speculos_port = url_content.port;
        }

        std::unique_ptr<hw::io::device_io_tcp> device_io = std::make_unique<hw::io::device_io_tcp>(global_io_context);
        device_io->connect(speculos_host.c_str(), speculos_port);
        global_ledger_device.emplace(std::move(device_io));
    }
    else
    {
        global_ledger_device.emplace();
    }

    global_ledger_account.emplace().create_from_device(*global_ledger_device);
}
//----------------------------------------------------------------------------------------------------------------------
const cryptonote::account_keys &get_global_ledger_account()
{
    return global_ledger_account->get_keys();
}
//----------------------------------------------------------------------------------------------------------------------
cryptonote::tx_source_entry gen_ringed_tx_source_entry(
    const rct::xmr_amount amount,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const bool is_ringct,
    const std::size_t ring_size)
{
    cryptonote::tx_source_entry src;
    src.real_output = crypto::rand_idx(ring_size);
    src.real_output_in_tx_index = crypto::rand_idx<std::uint64_t>(BULLETPROOF_MAX_OUTPUTS);
    src.amount = amount;
    src.rct = is_ringct;
    for (std::size_t ring_member_idx = 0; ring_member_idx < ring_size; ++ring_member_idx)
    {
        auto &output = src.outputs.emplace_back();
        output.first = ring_member_idx;

        if (ring_member_idx != src.real_output)
        {
            // fake output
            output.second = {rct::pkGen(), rct::pkGen()};
            continue;
        }

        // generate ephemeral tx pubkeys
        crypto::secret_key tx_privkey = rct::rct2sk(rct::skGen());
        const crypto::public_key base_1 = is_subaddress ? address.m_spend_public_key : crypto::get_G();
        const crypto::public_key tx_pubkey = rct::rct2pk(rct::scalarmultKey(rct::pk2rct(base_1),
            rct::sk2rct(tx_privkey)));
        if (is_subaddress)
        {
            src.real_out_additional_tx_keys.resize(src.real_output_in_tx_index + 1);
            src.real_out_additional_tx_keys.at(src.real_output_in_tx_index) = tx_pubkey;
            src.real_out_tx_key = rct::rct2pk(rct::pkGen());
        }
        else
        {
            src.real_out_tx_key = tx_pubkey;
        }

        // derive ECDH and one-time address
        crypto::key_derivation kd;
        assert(crypto::generate_key_derivation(address.m_view_public_key, tx_privkey, kd));
        crypto::public_key onetime_address;
        assert(crypto::derive_public_key(kd, src.real_output_in_tx_index,
            address.m_spend_public_key, onetime_address));

        // calculate amount blinding factor
        if (is_ringct)
        {
            crypto::secret_key sd;
            crypto::derivation_to_scalar(kd, src.real_output_in_tx_index, sd);
            src.mask = rct::genCommitmentMask(rct::sk2rct(sd));
        }
        else
        {
            src.mask = rct::I;
        }

        // add output
        const rct::key amount_commitment = rct::commit(amount, src.mask);
        output.second = {rct::pk2rct(onetime_address), amount_commitment};
    }

    return src;
}
//----------------------------------------------------------------------------------------------------------------------
} //namespace mock
