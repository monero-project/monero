// Copyright (c) 2025, The Monero Project
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

//pair header
#include "carrot_mock_helpers.h"

//local headers
#include "carrot_core/hash_functions.h"
#include "cryptonote_core/cryptonote_tx_utils.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot.mock"

namespace carrot
{
namespace mock
{
//----------------------------------------------------------------------------------------------------------------------
CarrotDestinationV1 mock_carrot_and_legacy_keys::cryptonote_address(const payment_id_t payment_id,
    const AddressDeriveType derive_type) const
{
    CarrotDestinationV1 addr;
    switch (resolve_derive_type(derive_type))
    {
    case AddressDeriveType::Carrot:
        make_carrot_integrated_address_v1(carrot_account_spend_pubkey,
            legacy_acb.get_keys().m_account_address.m_view_public_key,
            payment_id,
            addr);
        break;
    case AddressDeriveType::PreCarrot:
        make_carrot_integrated_address_v1(legacy_acb.get_keys().m_account_address.m_spend_public_key,
            legacy_acb.get_keys().m_account_address.m_view_public_key,
            payment_id,
            addr);
        break;
    default:
        throw std::runtime_error("address derive type not recognized");
    }
    return addr;
}
//----------------------------------------------------------------------------------------------------------------------
CarrotDestinationV1 mock_carrot_and_legacy_keys::subaddress(const subaddress_index_extended &subaddress_index) const
{
    if (!subaddress_index.index.is_subaddress())
        return cryptonote_address(null_payment_id, subaddress_index.derive_type);

    const cryptonote::account_keys &lkeys = legacy_acb.get_keys();

    CarrotDestinationV1 addr;
    cryptonote::account_public_address cnaddr;
    switch (resolve_derive_type(subaddress_index.derive_type))
    {
    case AddressDeriveType::Carrot:
        make_carrot_subaddress_v1(carrot_account_spend_pubkey,
            carrot_account_view_pubkey,
            s_generate_address_dev,
            subaddress_index.index.major,
            subaddress_index.index.minor,
            addr);
        break;
    case AddressDeriveType::PreCarrot:
        cnaddr =
            lkeys.m_device->get_subaddress(lkeys, {subaddress_index.index.major, subaddress_index.index.minor});
        addr = CarrotDestinationV1{
            .address_spend_pubkey = cnaddr.m_spend_public_key,
            .address_view_pubkey = cnaddr.m_view_public_key,
            .is_subaddress = true,
            .payment_id = null_payment_id
        };
        break;
    default:
        throw std::runtime_error("address derive type not recognized");
    }
    return addr;
}
//----------------------------------------------------------------------------------------------------------------------
std::unordered_map<crypto::public_key, cryptonote::subaddress_index> mock_carrot_and_legacy_keys::subaddress_map_cn() const
{
    std::unordered_map<crypto::public_key, cryptonote::subaddress_index> res;
    for (const auto &p : subaddress_map)
        if (p.second.derive_type == AddressDeriveType::PreCarrot)
            res.emplace(p.first, cryptonote::subaddress_index{p.second.index.major, p.second.index.minor});
    CHECK_AND_ASSERT_THROW_MES(!res.empty(),
        "mock_carrot_and_legacy_keys::subaddress_map_cn: subaddress map does not contain pre-carrot subaddresses");
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
void mock_carrot_and_legacy_keys::opening_for_subaddress(const subaddress_index_extended &subaddress_index,
    crypto::secret_key &address_privkey_g_out,
    crypto::secret_key &address_privkey_t_out,
    crypto::public_key &address_spend_pubkey_out) const
{
    const bool is_subaddress = subaddress_index.index.is_subaddress();
    const uint32_t major_index = subaddress_index.index.major;
    const uint32_t minor_index = subaddress_index.index.minor;

    const cryptonote::account_keys &lkeys = legacy_acb.get_keys();

    crypto::secret_key address_index_generator;
    crypto::secret_key subaddress_scalar;
    crypto::secret_key subaddress_extension;

    switch (resolve_derive_type(subaddress_index.derive_type))
    {
    case AddressDeriveType::Carrot:
        // s^j_gen = H_32[s_ga](j_major, j_minor)
        make_carrot_index_extension_generator(s_generate_address, major_index, minor_index, address_index_generator);

        if (is_subaddress)
        {
            // k^j_subscal = H_n(K_s, j_major, j_minor, s^j_gen)
            make_carrot_subaddress_scalar(carrot_account_spend_pubkey, address_index_generator, major_index, minor_index, subaddress_scalar);
        }
        else
        {
            // k^j_subscal = 1
            sc_1(to_bytes(subaddress_scalar));
        }

        // k^g_a = k_gi * k^j_subscal
        sc_mul(to_bytes(address_privkey_g_out), to_bytes(k_generate_image), to_bytes(subaddress_scalar));

        // k^t_a = k_ps * k^j_subscal
        sc_mul(to_bytes(address_privkey_t_out), to_bytes(k_prove_spend), to_bytes(subaddress_scalar));
        break;
    case AddressDeriveType::PreCarrot:
        // m = Hn(k_v || j_major || j_minor) if subaddress else 0
        subaddress_extension = is_subaddress
            ? lkeys.get_device().get_subaddress_secret_key(lkeys.m_view_secret_key, {major_index, minor_index})
            : crypto::null_skey;

        // k^g_a = k_s + m
        sc_add(to_bytes(address_privkey_g_out), to_bytes(lkeys.m_spend_secret_key), to_bytes(subaddress_extension));

        // k^t_a = 0
        memset(address_privkey_t_out.data, 0, sizeof(address_privkey_t_out));
        break;
    default:
        throw std::runtime_error("address derive type not recognized");
    }

    // perform sanity check
    const CarrotDestinationV1 addr = subaddress(subaddress_index);
    rct::key recomputed_address_spend_pubkey;
    rct::addKeys2(recomputed_address_spend_pubkey,
        rct::sk2rct(address_privkey_g_out),
        rct::sk2rct(address_privkey_t_out),
        rct::pk2rct(crypto::get_T()));
    CHECK_AND_ASSERT_THROW_MES(rct::rct2pk(recomputed_address_spend_pubkey) == addr.address_spend_pubkey,
        "mock carrot or legacy keys: opening for subaddress: failed sanity check");
    address_spend_pubkey_out = addr.address_spend_pubkey;
}
//----------------------------------------------------------------------------------------------------------------------
bool mock_carrot_and_legacy_keys::try_searching_for_opening_for_subaddress(const crypto::public_key &address_spend_pubkey,
    crypto::secret_key &address_privkey_g_out,
    crypto::secret_key &address_privkey_t_out) const
{
    const auto it = subaddress_map.find(address_spend_pubkey);
    if (it == subaddress_map.cend())
        return false;

    crypto::public_key recomputed_address_spend_pubkey;
    opening_for_subaddress(it->second,
        address_privkey_g_out,
        address_privkey_t_out,
        recomputed_address_spend_pubkey);

    return address_spend_pubkey == recomputed_address_spend_pubkey;
}
bool mock_carrot_and_legacy_keys::try_searching_for_opening_for_onetime_address(const crypto::public_key &address_spend_pubkey,
    const crypto::secret_key &sender_extension_g,
    const crypto::secret_key &sender_extension_t,
    crypto::secret_key &x_out,
    crypto::secret_key &y_out) const
{
    // k^{j,g}_addr, k^{j,t}_addr
    crypto::secret_key address_privkey_g;
    crypto::secret_key address_privkey_t;
    if (!try_searching_for_opening_for_subaddress(address_spend_pubkey,
            address_privkey_g,
            address_privkey_t))
        return false;

    // x = k^{j,g}_addr + k^g_o
    sc_add(to_bytes(x_out), to_bytes(address_privkey_g), to_bytes(sender_extension_g));

    // y = k^{j,t}_addr + k^t_o
    sc_add(to_bytes(y_out), to_bytes(address_privkey_t), to_bytes(sender_extension_t));

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
bool mock_carrot_and_legacy_keys::can_open_fcmp_onetime_address(const crypto::public_key &address_spend_pubkey,
    const crypto::secret_key &sender_extension_g,
    const crypto::secret_key &sender_extension_t,
    const crypto::public_key &onetime_address) const
{
    crypto::secret_key x, y;
    if (!try_searching_for_opening_for_onetime_address(address_spend_pubkey,
            sender_extension_g,
            sender_extension_t,
            x,
            y))
        return false;

    // O' = x G + y T
    rct::key recomputed_onetime_address;
    rct::addKeys2(recomputed_onetime_address,
        rct::sk2rct(x),
        rct::sk2rct(y),
        rct::pk2rct(crypto::get_T()));

    // O' ?= O
    return 0 == memcmp(&recomputed_onetime_address, &onetime_address, sizeof(rct::key));
}
//----------------------------------------------------------------------------------------------------------------------
crypto::key_image mock_carrot_and_legacy_keys::derive_key_image(const crypto::public_key &address_spend_pubkey,
    const crypto::secret_key &sender_extension_g,
    const crypto::secret_key &sender_extension_t,
    const crypto::public_key &onetime_address) const
{
    CHECK_AND_ASSERT_THROW_MES(can_open_fcmp_onetime_address(
            address_spend_pubkey,
            sender_extension_g,
            sender_extension_t,
            onetime_address),
        "mock carrot and legacy keys: derive key image: cannot open onetime address");

    crypto::secret_key x, y;
    try_searching_for_opening_for_onetime_address(address_spend_pubkey,
        sender_extension_g,
        sender_extension_t,
        x,
        y);

    crypto::key_image L;
    crypto::generate_key_image(onetime_address, x, L);
    return L;
}
//----------------------------------------------------------------------------------------------------------------------
void mock_carrot_and_legacy_keys::generate_subaddress_map()
{
    const std::vector<AddressDeriveType> derive_types{AddressDeriveType::Carrot, AddressDeriveType::PreCarrot};

    for (uint32_t major_index = 0; major_index <= MAX_SUBADDRESS_MAJOR_INDEX; ++major_index)
    {
        for (uint32_t minor_index = 0; minor_index <= MAX_SUBADDRESS_MINOR_INDEX; ++minor_index)
        {
            for (const AddressDeriveType derive_type : derive_types)
            {
                const subaddress_index_extended subaddr_index{{major_index, minor_index}, derive_type};
                const CarrotDestinationV1 addr = subaddress(subaddr_index);
                subaddress_map.insert({addr.address_spend_pubkey, subaddr_index});
            }
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
void mock_carrot_and_legacy_keys::generate(const AddressDeriveType default_derive_type)
{
    legacy_acb.generate();

    crypto::generate_random_bytes_thread_safe(sizeof(crypto::secret_key), to_bytes(s_master));
    make_carrot_provespend_key(s_master, k_prove_spend);
    make_carrot_viewbalance_secret(s_master, s_view_balance);
    make_carrot_generateimage_key(s_view_balance, k_generate_image);
    make_carrot_generateaddress_secret(s_view_balance, s_generate_address);

    make_carrot_spend_pubkey(k_generate_image, k_prove_spend, carrot_account_spend_pubkey);
    k_view_incoming_dev.view_key_scalar_mult_ed25519(carrot_account_spend_pubkey,
        carrot_account_view_pubkey);

    this->default_derive_type = default_derive_type;

    generate_subaddress_map();
}
//----------------------------------------------------------------------------------------------------------------------
AddressDeriveType mock_carrot_and_legacy_keys::resolve_derive_type(const AddressDeriveType derive_type) const
{
    return derive_type == AddressDeriveType::Auto ? default_derive_type : derive_type;
}
//----------------------------------------------------------------------------------------------------------------------
crypto::key_image dummy_key_image_device::derive_key_image(const OutputOpeningHintVariant &opening_hint) const
{
    static constexpr unsigned char domain_separator[]
        = "One for the money, two for the better green, 3,4-methylenedioxymethamphetamine";
    static_assert(sizeof(domain_separator) > 1);
    unsigned char data[sizeof(domain_separator) + sizeof(crypto::public_key)] = {0};
    memcpy(data, &domain_separator, sizeof(domain_separator));
    const crypto::public_key ota = onetime_address_ref(opening_hint);
    memcpy(data + sizeof(domain_separator), &ota, sizeof(ota));
    unsigned char h[32];
    derive_bytes_32(data, sizeof(data), nullptr, h);
    ge_p2 KI_p2;
    ge_fromfe_frombytes_vartime(&KI_p2, h); // fancy, if not really necessary
    crypto::key_image KI;
    ge_tobytes(to_bytes(KI), &KI_p2);
    return KI;
}
//----------------------------------------------------------------------------------------------------------------------
void mock_scan_enote_set(const std::vector<CarrotEnoteV1> &enotes,
    const encrypted_payment_id_t encrypted_payment_id,
    const mock_carrot_and_legacy_keys &keys,
    std::vector<mock_scan_result_t> &res)
{
    res.clear();

    // We support receives to both the new and old K^0_s
    const crypto::public_key main_address_spend_pubkeys[2] = {
        keys.carrot_account_spend_pubkey,
        keys.legacy_acb.get_keys().m_account_address.m_spend_public_key
    };

    // external scans
    for (size_t output_index = 0; output_index < enotes.size(); ++output_index)
    {
        const CarrotEnoteV1 &enote = enotes.at(output_index);

        // s_sr = k_v D_e
        mx25519_pubkey s_sr;
        if (!keys.k_view_incoming_dev.view_key_scalar_mult_x25519(enote.enote_ephemeral_pubkey, s_sr))
            continue;

        mock_scan_result_t scan_result{};
        scan_result.output_index = output_index;

        if (try_scan_carrot_enote_external_receiver(enote,
            encrypted_payment_id,
            s_sr,
            {main_address_spend_pubkeys, 2},
            keys.k_view_incoming_dev,
            scan_result.sender_extension_g,
            scan_result.sender_extension_t,
            scan_result.address_spend_pubkey,
            scan_result.amount,
            scan_result.amount_blinding_factor,
            scan_result.payment_id,
            scan_result.enote_type))
        {
            res.push_back(scan_result);
            continue;
        }
    }

    // internal scans
    for (size_t output_index = 0; output_index < enotes.size(); ++output_index)
    {
        const CarrotEnoteV1 &enote = enotes.at(output_index);

        mock_scan_result_t scan_result{};
        const bool r = try_scan_carrot_enote_internal_receiver(enote,
            keys.s_view_balance_dev,
            scan_result.sender_extension_g,
            scan_result.sender_extension_t,
            scan_result.address_spend_pubkey,
            scan_result.amount,
            scan_result.amount_blinding_factor,
            scan_result.enote_type,
            scan_result.internal_message);

        scan_result.output_index = output_index;

        if (r)
            res.push_back(scan_result);
    }
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
void mock_scan_coinbase_enote_set(const std::vector<CarrotCoinbaseEnoteV1> &coinbase_enotes,
    const mock_carrot_and_legacy_keys &keys,
    std::vector<mock_scan_result_t> &res)
{
    res.clear();

    // We support receives to both the new and old K^0_s
    const crypto::public_key main_address_spend_pubkeys[2] = {
        keys.carrot_account_spend_pubkey,
        keys.legacy_acb.get_keys().m_account_address.m_spend_public_key
    };

    for (size_t output_index = 0; output_index < coinbase_enotes.size(); ++output_index)
    {
        const CarrotCoinbaseEnoteV1 &enote = coinbase_enotes.at(output_index);

        mock_scan_result_t scan_result{};
        scan_result.output_index = output_index;
        scan_result.amount = enote.amount;
        sc_1(to_bytes(scan_result.amount_blinding_factor));
        scan_result.payment_id = null_payment_id;
        scan_result.enote_type = CarrotEnoteType::PAYMENT;
        scan_result.internal_message = janus_anchor_t{};

        mx25519_pubkey s_sender_receiver_unctx;
        make_carrot_uncontextualized_shared_key_receiver(keys.k_view_incoming_dev,
            enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        if (try_scan_carrot_coinbase_enote_receiver(enote,
            s_sender_receiver_unctx,
            {main_address_spend_pubkeys, 2},
            scan_result.sender_extension_g,
            scan_result.sender_extension_t,
            scan_result.address_spend_pubkey))
        {
            res.push_back(scan_result);
            continue;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
bool compare_scan_result(const mock_scan_result_t &scan_res,
    const CarrotPaymentProposalV1 &normal_payment_proposal,
    const rct::xmr_amount allowed_fee_margin_opt)
{
    if (scan_res.address_spend_pubkey != normal_payment_proposal.destination.address_spend_pubkey)
        return false;

    if (scan_res.amount > normal_payment_proposal.amount)
        return false;

    if (normal_payment_proposal.amount - scan_res.amount > allowed_fee_margin_opt)
        return false;

    if (scan_res.enote_type != CarrotEnoteType::PAYMENT)
        return false;

    if (scan_res.payment_id != normal_payment_proposal.destination.payment_id)
        return false;

    if (scan_res.internal_message != janus_anchor_t{})
        return false;

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
bool compare_scan_result(const mock_scan_result_t &scan_res,
    const CarrotPaymentProposalSelfSendV1 &selfsend_payment_proposal,
    const rct::xmr_amount allowed_fee_margin_opt)
{
    if (scan_res.address_spend_pubkey != selfsend_payment_proposal.destination_address_spend_pubkey)
        return false;

    if (scan_res.amount > selfsend_payment_proposal.amount)
        return false;

    if (selfsend_payment_proposal.amount - scan_res.amount > allowed_fee_margin_opt)
        return false;

    if (scan_res.enote_type != selfsend_payment_proposal.enote_type)
        return false;

    if (scan_res.payment_id != null_payment_id)
        return false;

    if (scan_res.internal_message != selfsend_payment_proposal.internal_message.value_or(janus_anchor_t{}))
        return false;

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
crypto::public_key gen_public_key()
{
    return rct::rct2pk(rct::pkGen());
}
//----------------------------------------------------------------------------------------------------------------------
crypto::key_image gen_key_image()
{
    return rct::rct2ki(rct::pkGen());
}
//----------------------------------------------------------------------------------------------------------------------
crypto::secret_key gen_secret_key()
{
    return rct::rct2sk(rct::skGen());
}
//----------------------------------------------------------------------------------------------------------------------
subaddress_index gen_subaddress_index()
{
    // guaranteed not to return the main address index
    return {crypto::rand_range<uint32_t>(1, MAX_SUBADDRESS_MAJOR_INDEX),
            crypto::rand_range<uint32_t>(1, MAX_SUBADDRESS_MINOR_INDEX)};
}
//----------------------------------------------------------------------------------------------------------------------
subaddress_index_extended gen_subaddress_index_extended(const AddressDeriveType derive_type)
{
    return {gen_subaddress_index(), derive_type};
}
//----------------------------------------------------------------------------------------------------------------------
CarrotEnoteV1 gen_carrot_enote_v1()
{
    return CarrotEnoteV1{
        .onetime_address = gen_public_key(),
        .amount_commitment = rct::pkGen(),
        .amount_enc = gen_encrypted_amount(),
        .anchor_enc = gen_janus_anchor(),
        .view_tag = gen_view_tag(),
        .enote_ephemeral_pubkey = gen_x25519_pubkey(),
        .tx_first_key_image = gen_key_image()
    };
}
//----------------------------------------------------------------------------------------------------------------------
CarrotCoinbaseEnoteV1 gen_carrot_coinbase_enote_v1()
{
    return CarrotCoinbaseEnoteV1{
        .onetime_address = gen_public_key(),
        .amount = COIN + rct::randXmrAmount(COIN),
        .anchor_enc = gen_janus_anchor(),
        .view_tag = gen_view_tag(),
        .enote_ephemeral_pubkey = gen_x25519_pubkey(),
        .block_index = crypto::rand_idx<std::uint64_t>(CRYPTONOTE_MAX_BLOCK_NUMBER)
    };
}
//----------------------------------------------------------------------------------------------------------------------
std::vector<CarrotEnoteV1> collect_enotes(const std::vector<RCTOutputEnoteProposal> &output_enote_proposals)
{
    std::vector<CarrotEnoteV1> res;
    res.reserve(output_enote_proposals.size());
    for (const RCTOutputEnoteProposal &output_enote_proposal : output_enote_proposals)
        res.push_back(output_enote_proposal.enote);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
std::uint64_t gen_block_index()
{
    return crypto::rand_idx<std::uint64_t>(CRYPTONOTE_MAX_BLOCK_NUMBER);
}
//----------------------------------------------------------------------------------------------------------------------
CarrotDestinationV1 convert_destination_v1(const cryptonote::tx_destination_entry &cn_dst)
{
    return CarrotDestinationV1{
        .address_spend_pubkey = cn_dst.addr.m_spend_public_key,
        .address_view_pubkey = cn_dst.addr.m_view_public_key,
        .is_subaddress = cn_dst.is_subaddress,
        .payment_id = null_payment_id // payment ID provided elsewhere
    };
}
//----------------------------------------------------------------------------------------------------------------------
cryptonote::tx_destination_entry convert_destination_v1(const CarrotDestinationV1 &dst, const rct::xmr_amount amount)
{
    return cryptonote::tx_destination_entry(amount,
        {dst.address_spend_pubkey, dst.address_view_pubkey},
        dst.is_subaddress);
}
//----------------------------------------------------------------------------------------------------------------------
CarrotPaymentProposalV1 convert_normal_payment_proposal_v1(const cryptonote::tx_destination_entry &cn_dst,
    const janus_anchor_t randomness)
{
    return CarrotPaymentProposalV1{
        .destination = convert_destination_v1(cn_dst),
        .amount = cn_dst.amount,
        .randomness = randomness
    };
}
//----------------------------------------------------------------------------------------------------------------------
CarrotPaymentProposalSelfSendV1 convert_selfsend_payment_proposal_v1(const cryptonote::tx_destination_entry &cn_dst)
{
    return CarrotPaymentProposalSelfSendV1{
        .destination_address_spend_pubkey = cn_dst.addr.m_spend_public_key,
        .amount = cn_dst.amount,
        .enote_type = CarrotEnoteType::PAYMENT,
        .enote_ephemeral_pubkey = std::nullopt,
        .internal_message = std::nullopt
    };
}
//----------------------------------------------------------------------------------------------------------------------
} //namespace mock
} //namespace carrot