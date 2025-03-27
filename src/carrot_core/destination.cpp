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
#include "misc_log_ex.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot"

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
    CHECK_AND_ASSERT_THROW_MES(j_major != 0 || j_minor,
        "make carrot subaddress v1: j cannot be 0 for a subaddress, only for main addresses");

    // s^j_gen = H_32[s_ga](j_major, j_minor)
    crypto::secret_key address_index_generator;
    s_generate_address_dev.make_index_extension_generator(j_major, j_minor, address_index_generator);

    // k^j_subscal = H_n(K_s, j_major, j_minor, s^j_gen)
    crypto::secret_key subaddress_scalar;
    make_carrot_subaddress_scalar(account_spend_pubkey, address_index_generator, j_major, j_minor, subaddress_scalar);

    // K^j_s = k^j_subscal * K_s
    const rct::key address_spend_pubkey =
        rct::scalarmultKey(rct::pk2rct(account_spend_pubkey), rct::sk2rct(subaddress_scalar));
    
    // K^j_v = k^j_subscal * K_v
    const rct::key address_view_pubkey =
        rct::scalarmultKey(rct::pk2rct(account_view_pubkey), rct::sk2rct(subaddress_scalar));

    destination_out = CarrotDestinationV1{
        .address_spend_pubkey = rct::rct2pk(address_spend_pubkey),
        .address_view_pubkey = rct::rct2pk(address_view_pubkey),
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
        .address_spend_pubkey = rct::rct2pk(rct::pkGen()),
        .address_view_pubkey = rct::rct2pk(rct::pkGen()),
        .is_subaddress = false,
        .payment_id = null_payment_id
    };
}
//-------------------------------------------------------------------------------------------------------------------
CarrotDestinationV1 gen_carrot_subaddress_v1()
{
    return CarrotDestinationV1{
        .address_spend_pubkey = rct::rct2pk(rct::pkGen()),
        .address_view_pubkey = rct::rct2pk(rct::pkGen()),
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
        .address_spend_pubkey = rct::rct2pk(rct::pkGen()),
        .address_view_pubkey = rct::rct2pk(rct::pkGen()),
        .is_subaddress = false,
        .payment_id = payment_id
    };
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
