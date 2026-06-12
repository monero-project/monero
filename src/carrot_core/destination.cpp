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
#include "destination.h"

//local headers
#include "address_utils.h"
#include "crypto/crypto.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "exceptions.h"
#include "misc_log_ex.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot.dest"

namespace {
crypto::public_key genPk()
{
    crypto::public_key s;
    crypto::random32_unbiased(to_bytes(s));
    ge_p3 P;
    ge_scalarmult_base(&P, to_bytes(s));
    ge_p3_tobytes(to_bytes(s), &P);
    return s;    
}
} //anonymous namespace

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const CarrotDestinationV1 &a, const CarrotDestinationV1 &b)
{
    return a.address_spend_pubkey == b.address_spend_pubkey &&
           a.address_view_pubkey  == b.address_view_pubkey &&
           a.is_subaddress        == b.is_subaddress &&
           a.payment_id           == b.payment_id;
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_main_address_v1(const crypto::public_key &account_spend_pubkey,
    const crypto::public_key &primary_address_view_pubkey,
    CarrotDestinationV1 &destination_out)
{
    destination_out = CarrotDestinationV1{
        .address_spend_pubkey = account_spend_pubkey,
        .address_view_pubkey = primary_address_view_pubkey,
        .is_subaddress = false,
        .payment_id = null_payment_id
    };
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_subaddress_v1(const crypto::public_key &account_spend_pubkey,
    const crypto::public_key &account_view_pubkey,
    const generate_address_secret_device &s_generate_address_dev,
    const std::uint32_t j_major,
    const std::uint32_t j_minor,
    CarrotDestinationV1 &destination_out)
{
    CARROT_CHECK_AND_THROW(j_major || j_minor,
        bad_address_type, "j cannot be 0 for a subaddress, only for main addresses");

    // s^j_ap1 = H_32[s_ga](j_major, j_minor)
    crypto::secret_key address_index_preimage_1;
    s_generate_address_dev.make_address_index_preimage_1(j_major, j_minor, address_index_preimage_1);

    // s^j_ap2 = H_32[s^j_ap1](j_major, j_minor, K_s, K_v)
    crypto::secret_key address_index_preimage_2;
    make_carrot_address_index_preimage_2(address_index_preimage_1,
        j_major,
        j_minor,
        account_spend_pubkey,
        account_view_pubkey,
        address_index_preimage_2);

    // k^j_subscal = H_n[s^j_ap2](K_s)
    crypto::secret_key subaddress_scalar;
    make_carrot_subaddress_scalar(address_index_preimage_2, account_spend_pubkey, subaddress_scalar);

    // K^j_s = k^j_subscal * K_s
    ge_p3 tmp1;
    CARROT_CHECK_AND_THROW(0 == ge_frombytes_vartime(&tmp1, to_bytes(account_spend_pubkey)),
        invalid_point, "invalid account spend pubkey");
    ge_p2 tmp2;
    ge_scalarmult(&tmp2, to_bytes(subaddress_scalar), &tmp1);
    crypto::public_key address_spend_pubkey;
    ge_tobytes(to_bytes(address_spend_pubkey), &tmp2);

    // K^j_v = k^j_subscal * K_v
    CARROT_CHECK_AND_THROW(0 == ge_frombytes_vartime(&tmp1, to_bytes(account_view_pubkey)),
        invalid_point, "invalid account view pubkey");
    ge_scalarmult(&tmp2, to_bytes(subaddress_scalar), &tmp1);
    crypto::public_key address_view_pubkey;
    ge_tobytes(to_bytes(address_view_pubkey), &tmp2);

    destination_out = CarrotDestinationV1{
        .address_spend_pubkey = address_spend_pubkey,
        .address_view_pubkey = address_view_pubkey,
        .is_subaddress = true,
        .payment_id = null_payment_id
    };
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_integrated_address_v1(const crypto::public_key &account_spend_pubkey,
    const crypto::public_key &primary_address_view_pubkey,
    const payment_id_t payment_id,
    CarrotDestinationV1 &destination_out)
{
    destination_out = CarrotDestinationV1{
        .address_spend_pubkey = account_spend_pubkey,
        .address_view_pubkey = primary_address_view_pubkey,
        .is_subaddress = false,
        .payment_id = payment_id
    };
}
//-------------------------------------------------------------------------------------------------------------------
CarrotDestinationV1 gen_carrot_main_address_v1()
{
    return CarrotDestinationV1{
        .address_spend_pubkey = genPk(),
        .address_view_pubkey = genPk(),
        .is_subaddress = false,
        .payment_id = null_payment_id
    };
}
//-------------------------------------------------------------------------------------------------------------------
CarrotDestinationV1 gen_carrot_subaddress_v1()
{
    return CarrotDestinationV1{
        .address_spend_pubkey = genPk(),
        .address_view_pubkey = genPk(),
        .is_subaddress = true,
        .payment_id = null_payment_id
    };
}
//-------------------------------------------------------------------------------------------------------------------
CarrotDestinationV1 gen_carrot_integrated_address_v1()
{
    // force generate non-zero payment id
    payment_id_t payment_id{gen_payment_id()};
    while (payment_id == null_payment_id)
        payment_id = gen_payment_id();

    return CarrotDestinationV1{
        .address_spend_pubkey = genPk(),
        .address_view_pubkey = genPk(),
        .is_subaddress = false,
        .payment_id = payment_id
    };
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
