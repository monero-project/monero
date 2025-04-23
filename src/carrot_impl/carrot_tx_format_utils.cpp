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

//paired header
#include "carrot_tx_format_utils.h"

//local headers
#include "common/container_helpers.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_config.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

static_assert(sizeof(mx25519_pubkey) == sizeof(crypto::public_key),
    "cannot use crypto::public_key as storage for X25519 keys since size is different");

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <class EnoteContainer>
static void store_carrot_ephemeral_pubkeys_to_extra(const EnoteContainer &enotes, std::vector<uint8_t> &extra_inout)
{
    const size_t nouts = enotes.size();
    const bool use_shared_ephemeral_pubkey = nouts == 2 && 0 == memcmp(
            &enotes.front().enote_ephemeral_pubkey,
            &enotes.back().enote_ephemeral_pubkey, 
            sizeof(mx25519_pubkey));
    bool success = true;
    if (use_shared_ephemeral_pubkey)
    {
        const mx25519_pubkey &enote_ephemeral_pubkey = enotes.at(0).enote_ephemeral_pubkey;
        const crypto::public_key tx_pubkey = raw_byte_convert<crypto::public_key>(enote_ephemeral_pubkey);
        success = success && cryptonote::add_tx_pub_key_to_extra(extra_inout, tx_pubkey);
    }
    else // !use_shared_ephemeral_pubkey
    {
        std::vector<crypto::public_key> tx_pubkeys(nouts);
        for (size_t i = 0; i < nouts; ++i)
        {
            const mx25519_pubkey &enote_ephemeral_pubkey = enotes.at(i).enote_ephemeral_pubkey;
            tx_pubkeys[i] = raw_byte_convert<crypto::public_key>(enote_ephemeral_pubkey);
        }
        success = success && cryptonote::add_additional_tx_pub_keys_to_extra(extra_inout, tx_pubkeys);
    }
    CHECK_AND_ASSERT_THROW_MES(success, "add carrot ephemeral pubkeys to extra: failed to add tx_extra fields");
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static mx25519_pubkey &De_ref(CarrotCoinbaseEnoteV1 &e) { return e.enote_ephemeral_pubkey; }
static mx25519_pubkey &De_ref(CarrotEnoteV1 &e) { return e.enote_ephemeral_pubkey; }
static mx25519_pubkey &De_ref(mx25519_pubkey &e) { return e; }
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <typename EnoteType>
static bool try_load_carrot_ephemeral_pubkeys_from_extra(const std::vector<cryptonote::tx_extra_field> &extra_fields,
    std::vector<EnoteType> &enotes_inout)
{
    cryptonote::tx_extra_pub_key tx_pubkey;
    cryptonote::tx_extra_additional_pub_keys tx_pubkeys;
    if (cryptonote::find_tx_extra_field_by_type(extra_fields, tx_pubkey))
    {
        De_ref(enotes_inout.front()) = raw_byte_convert<mx25519_pubkey>(tx_pubkey.pub_key);
        De_ref(enotes_inout.back()) = raw_byte_convert<mx25519_pubkey>(tx_pubkey.pub_key);
        return true;
    }
    else if (cryptonote::find_tx_extra_field_by_type(extra_fields, tx_pubkeys))
    {
        if (tx_pubkeys.data.size() != enotes_inout.size())
            return false;

        for (size_t i = 0; i < enotes_inout.size(); ++i)
            De_ref(enotes_inout[i]) = raw_byte_convert<mx25519_pubkey>(tx_pubkeys.data.at(i));

        return true;
    }

    return false;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
bool is_carrot_transaction_v1(const cryptonote::transaction_prefix &tx_prefix)
{
    return tx_prefix.vout.at(0).target.type() == typeid(cryptonote::txout_to_carrot_v1);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_load_carrot_extra_v1(
    const std::vector<cryptonote::tx_extra_field> &tx_extra_fields,
    std::vector<mx25519_pubkey> &enote_ephemeral_pubkeys_out,
    std::optional<encrypted_payment_id_t> &encrypted_payment_id_out)
{
    //ephemeral pubkeys: D_e
    if (!try_load_carrot_ephemeral_pubkeys_from_extra(tx_extra_fields, enote_ephemeral_pubkeys_out))
        return false;

    //encrypted payment ID: pid_enc
    encrypted_payment_id_out = std::nullopt;
    cryptonote::tx_extra_nonce extra_nonce;
    if (cryptonote::find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
    {
        crypto::hash8 pid_enc_8;
        if (cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, pid_enc_8))
        {
            encrypted_payment_id_t &pid_enc = encrypted_payment_id_out.emplace();
            pid_enc = raw_byte_convert<encrypted_payment_id_t>(pid_enc_8);
        }
    }

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
cryptonote::transaction store_carrot_to_transaction_v1(const std::vector<CarrotEnoteV1> &enotes,
    const std::vector<crypto::key_image> &key_images,
    const rct::xmr_amount fee,
    const encrypted_payment_id_t encrypted_payment_id)
{
    const size_t nins = key_images.size();
    const size_t nouts = enotes.size();

    cryptonote::transaction tx;
    tx.pruned = true;
    tx.version = 2;
    tx.unlock_time = 0;
    tx.vin.reserve(nins);
    tx.vout.reserve(nouts);
    tx.extra.reserve(MAX_TX_EXTRA_SIZE);
    tx.rct_signatures.type = carrot_v1_rct_type;
    tx.rct_signatures.txnFee = fee;
    tx.rct_signatures.ecdhInfo.reserve(nouts);
    tx.rct_signatures.outPk.reserve(nouts);

    //inputs
    for (const crypto::key_image &ki : key_images)
    {
        //L
        tx.vin.emplace_back(cryptonote::txin_to_key{ //@TODO: can save 2 bytes by using slim input type
            .amount = 0,
            .key_offsets = {},
            .k_image = ki
        });
    }

    //outputs
    for (const CarrotEnoteV1 &enote : enotes)
    {
        //K_o,vt,anchor_enc
        tx.vout.push_back(cryptonote::tx_out{0, cryptonote::txout_to_carrot_v1{
            .key = enote.onetime_address,
            .view_tag = enote.view_tag,
            .encrypted_janus_anchor = enote.anchor_enc
        }});

        //a_enc
        rct::ecdhTuple &ecdh_tuple = tools::add_element(tx.rct_signatures.ecdhInfo);
        memcpy(ecdh_tuple.amount.bytes, enote.amount_enc.bytes, sizeof(ecdh_tuple.amount));

        //C_a
        tx.rct_signatures.outPk.push_back(rct::ctkey{rct::key{}, enote.amount_commitment});
    }

    //ephemeral pubkeys: D_e
    store_carrot_ephemeral_pubkeys_to_extra(enotes, tx.extra);

    //encrypted payment id: pid_enc
    const crypto::hash8 pid_enc_8 = raw_byte_convert<crypto::hash8>(encrypted_payment_id);
    cryptonote::blobdata extra_nonce;
    cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, pid_enc_8);
    CHECK_AND_ASSERT_THROW_MES(cryptonote::add_extra_nonce_to_tx_extra(tx.extra, extra_nonce),
        "store carrot to transaction v1: failed to add encrypted payment ID to tx_extra");

    //finalize tx_extra
    CHECK_AND_ASSERT_THROW_MES(cryptonote::sort_tx_extra(tx.extra, tx.extra, /*allow_partial=*/false),
        "store carrot to transaction v1: failed to sort tx_extra");

    return tx;
}
//-------------------------------------------------------------------------------------------------------------------
bool try_load_carrot_from_transaction_v1(const cryptonote::transaction &tx,
    std::vector<CarrotEnoteV1> &enotes_out,
    std::vector<crypto::key_image> &key_images_out,
    rct::xmr_amount &fee_out,
    std::optional<encrypted_payment_id_t> &encrypted_payment_id_out)
{
    const rct::rctSigBase &rv = tx.rct_signatures;
    fee_out = rv.txnFee;

    const size_t nins = tx.vin.size();
    const size_t nouts = tx.vout.size();

    if (0 == nins)
        return false; // no input_context
    else if (nouts != rv.ecdhInfo.size())
        return false; // incorrect # of encrypted amounts
    else if (nouts != rv.outPk.size())
        return false; // incorrect # of amount commitments

    //inputs
    key_images_out.resize(nins);
    for (size_t i = 0; i < nins; ++i)
    {
        const cryptonote::txin_to_key * const k = boost::strict_get<cryptonote::txin_to_key>(&tx.vin.at(i));
        if (nullptr == k)
            return false;

        //L
        key_images_out[i] = k->k_image;
    }

    //outputs
    enotes_out.resize(nouts);
    for (size_t i = 0; i < nouts; ++i)
    {
        const cryptonote::txout_target_v &t = tx.vout.at(i).target;
        const cryptonote::txout_to_carrot_v1 * const c = boost::strict_get<cryptonote::txout_to_carrot_v1>(&t);
        if (nullptr == c)
            return false;
        
        //K_o
        enotes_out[i].onetime_address = c->key;

        //vt
        enotes_out[i].view_tag = c->view_tag;

        //anchor_enc
        enotes_out[i].anchor_enc = c->encrypted_janus_anchor;

        //L_1
        enotes_out[i].tx_first_key_image = key_images_out.at(0);

        //a_enc
        memcpy(enotes_out[i].amount_enc.bytes, rv.ecdhInfo.at(i).amount.bytes, sizeof(encrypted_amount_t));
    
        //C_a
        enotes_out[i].amount_commitment = rv.outPk.at(i).mask;
    }

    //parse tx_extra
    std::vector<cryptonote::tx_extra_field> extra_fields;
    if (!cryptonote::parse_tx_extra(tx.extra, extra_fields))
        return false;

    //ephemeral pubkeys: D_e
    if (!try_load_carrot_ephemeral_pubkeys_from_extra(extra_fields, enotes_out))
        return false;

    //encrypted payment ID: pid_enc
    encrypted_payment_id_out = std::nullopt;
    cryptonote::tx_extra_nonce extra_nonce;
    if (cryptonote::find_tx_extra_field_by_type(extra_fields, extra_nonce))
    {
        crypto::hash8 pid_enc_8;
        if (cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, pid_enc_8))
            encrypted_payment_id_out.emplace() = raw_byte_convert<encrypted_payment_id_t>(pid_enc_8);
    }

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
cryptonote::transaction store_carrot_to_coinbase_transaction_v1(
    const std::vector<CarrotCoinbaseEnoteV1> &enotes)
{
    const size_t nouts = enotes.size();
    const std::uint64_t block_index = enotes.at(0).block_index;

    cryptonote::transaction tx;
    tx.pruned = false;
    tx.version = 2;
    tx.unlock_time = block_index + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;
    tx.vin.reserve(1);
    tx.vout.reserve(nouts);
    tx.extra.reserve(MAX_TX_EXTRA_SIZE);
    tx.rct_signatures.type = rct::RCTTypeNull;

    //input
    tx.vin.emplace_back(cryptonote::txin_gen{.height = static_cast<size_t>(block_index)});

    //outputs
    for (const CarrotCoinbaseEnoteV1 &enote : enotes)
    {
        //K_o,vt,anchor_enc,a
        tx.vout.push_back(cryptonote::tx_out{enote.amount,
            cryptonote::txout_to_carrot_v1{
                .key = enote.onetime_address,
                .view_tag = enote.view_tag,
                .encrypted_janus_anchor = enote.anchor_enc
            }
        });
    }

    //ephemeral pubkeys: D_e
    store_carrot_ephemeral_pubkeys_to_extra(enotes, tx.extra);

    //we don't need to sort tx_extra since we only added one field
    //if you add more tx_extra fields here in the future, then please sort <3

    return tx;
}
//-------------------------------------------------------------------------------------------------------------------
bool try_load_carrot_from_coinbase_transaction_v1(const cryptonote::transaction &tx,
    std::vector<CarrotCoinbaseEnoteV1> &enotes_out)
{
    const size_t nins = tx.vin.size();
    const size_t nouts = tx.vout.size();

    if (1 == nins)
        return false; // not coinbase

    //input
    const cryptonote::txin_gen * const h = boost::strict_get<cryptonote::txin_gen>(&tx.vin.front());
    if (nullptr == h)
        return false;

    //outputs
    enotes_out.resize(nouts);
    for (size_t i = 0; i < nouts; ++i)
    {
        //a
        enotes_out[i].amount = tx.vout.at(i).amount;

        const cryptonote::txout_target_v &t = tx.vout.at(i).target;
        const cryptonote::txout_to_carrot_v1 * const c = boost::strict_get<cryptonote::txout_to_carrot_v1>(&t);
        if (nullptr == c)
            return false;

        //K_o
        enotes_out[i].onetime_address = c->key;

        //vt
        enotes_out[i].view_tag = c->view_tag;

        //anchor_enc
        enotes_out[i].anchor_enc = c->encrypted_janus_anchor;

        //block_index
        enotes_out[i].block_index = h->height;
    }

    //parse tx_extra
    std::vector<cryptonote::tx_extra_field> extra_fields;
    if (!cryptonote::parse_tx_extra(tx.extra, extra_fields))
        return false;

    //ephemeral pubkeys: D_e
    if (!try_load_carrot_ephemeral_pubkeys_from_extra(extra_fields, enotes_out))
        return false;

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
