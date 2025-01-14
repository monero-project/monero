// Copyright (c) 2014-2025, The Monero Project
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

#include "enote_details.h"


namespace Monero {

EnoteDetails::~EnoteDetails() {}


EnoteDetailsImpl::EnoteDetailsImpl():
    m_block_height(0),
    m_internal_enote_index(0),
    m_global_enote_index(0),
    m_spent(false),
    m_frozen(false),
    m_spent_height(0),
    m_amount(0),
    m_protocol_version(TxProtocol_CryptoNote),
    m_key_image_known(false),
    m_key_image_request(false),
    m_pk_index(0),
    m_key_image_partial(false)
{
}

EnoteDetailsImpl::~EnoteDetailsImpl() {}

std::string EnoteDetailsImpl::onetimeAddress() const
{ return m_onetime_address; }
std::string EnoteDetailsImpl::viewTag() const
{ return m_view_tag; }
std::uint64_t EnoteDetailsImpl::blockHeight() const
{ return m_block_height; }
std::string EnoteDetailsImpl::txId() const
{ return m_tx_id; }
std::uint64_t EnoteDetailsImpl::internalEnoteIndex() const
{ return m_internal_enote_index; }
std::uint64_t EnoteDetailsImpl::globalEnoteIndex() const
{ return m_global_enote_index; }
bool EnoteDetailsImpl::isSpent() const
{ return m_spent; }
bool EnoteDetailsImpl::isFrozen() const
{ return m_frozen; }
std::uint64_t EnoteDetailsImpl::spentHeight() const
{ return m_spent_height; }
std::string EnoteDetailsImpl::keyImage() const
{ return m_key_image; }
std::string EnoteDetailsImpl::mask() const
{ return m_mask; }
std::uint64_t EnoteDetailsImpl::amount() const
{ return m_amount; }
EnoteDetails::TxProtocol EnoteDetailsImpl::protocolVersion() const
{ return m_protocol_version; }
bool EnoteDetailsImpl::isKeyImageKnown() const
{ return m_key_image_known; }
bool EnoteDetailsImpl::isKeyImageRequest() const
{ return m_key_image_request; }
std::uint64_t EnoteDetailsImpl::pkIndex() const
{ return m_pk_index; }
std::vector<std::pair<std::uint64_t, std::string>> EnoteDetailsImpl::uses() const
{ return m_uses; }

// Multisig
bool EnoteDetailsImpl::isKeyImagePartial() const
{ return m_key_image_partial; }

} // namespace
