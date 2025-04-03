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

//! @file Abstract interfaces for performing scanning without revealing account keys

#pragma once

//local headers
#include "core_types.h"
#include "crypto/crypto.h"
#include "mx25519.h"

//third party headers

//standard headers
#include <cstdio>
#include <stdexcept>
#include <string>

//forward declarations


/**
 * These device interfaces were written primarily to be as lean as possible, for ease of the
 * implementor, so long as account keys don't leak. As such, the interfaces do not shield
 * sender-receiver secrets, and thus temporary access to this device interface can expose
 * transaction content permanently in a provable manner. The device interface currently used in
 * Monero (hw::device) also exposes transaction content, which can be saved permanently, but it
 * wouldn't necessarily be provable. Thus, in the case of a breach, the original user has some
 * plausible deniability with (hw::device), which cannot be said of the interfaces in this file.
 * It's not impossible to make carrot scanning happen completely on-device, but it is significantly
 * more involved.
 */

namespace carrot
{
/**
 * brief: base exception type for reporting carrot device errors.
 * note: devices should only throw this exception or derived classes
 */
struct device_error: public std::runtime_error
{
    /**
     * param: dev_make - e.g. "Trezor", "Ledger"
     * param: dev_model - e.g. "Model T", "Nano X"
     * param: func_called - verbatim device interface method name, e.g. "view_key_scalar_mult_x25519"
     * param: msg - arbitrary error message
     * param: code - arbitrary error code
     */
    device_error(std::string &&dev_make,
            std::string &&dev_model,
            std::string &&func_called,
            std::string &&msg,
            const int code)
        : std::runtime_error(make_formatted_message(dev_make, dev_model, func_called, msg, code)), 
            dev_make(dev_make), dev_model(dev_model), func_called(func_called), msg(msg), code(code)
    {}

    static std::string make_formatted_message(const std::string &dev_make,
        const std::string &dev_model,
        const std::string &func_called,
        const std::string &msg,
        const int code)
    {
        char buf[384];
        snprintf(buf, sizeof(buf),
            "%s %s device error (%d), at %s(): %s",
            dev_make.c_str(), dev_model.c_str(), code, func_called.c_str(), msg.c_str());
        return {buf};
    }

    const std::string dev_make;
    const std::string dev_model; 
    const std::string func_called;
    const std::string msg;
    const int code;
};

struct view_incoming_key_device
{
    /**
     * brief: view_key_scalar_mult_ed25519 - do an Ed25519 scalar mult against the incoming view key
     *   kvP = k_v * P
     * param: P - Ed25519 base point
     * outparam: kvP
     * return: true on success, false on failure (e.g. unable to decompress point)
     */
    virtual bool view_key_scalar_mult_ed25519(const crypto::public_key &P, crypto::public_key &kvP) const = 0;

    /**
     * brief: view_key_scalar_mult_x25519 - do an X25519 scalar mult and cofactor clear against the incoming view key
     *   kvD = k_v * D
     * param: D - X25519 base point
     * outparam: kvD
     * return: true on success, false on failure (e.g. unable to decompress point)
     */
    virtual bool view_key_scalar_mult_x25519(const mx25519_pubkey &D, mx25519_pubkey &kvD) const = 0;

    /**
     * brief: make_janus_anchor_special - make a janus anchor for "special" enotes
     *   anchor_sp = H_16(D_e, input_context, Ko, k_v)
     * param: enote_ephemeral_pubkey - D_e
     * param: input_context - input_context
     * param: account_spend_pubkey - K_s
     * outparam: anchor_special_out - anchor_sp
     */
    virtual void make_janus_anchor_special(const mx25519_pubkey &enote_ephemeral_pubkey,
        const input_context_t &input_context,
        const crypto::public_key &onetime_address,
        janus_anchor_t &anchor_special_out) const = 0;

    virtual ~view_incoming_key_device() = default;
};

struct view_balance_secret_device
{
    /**
     * brief: make_internal_view_tag - make an internal view tag, given non-secret data
     *   vt = H_3(s_vb || input_context || Ko)
     * param: input_context - input_context
     * param: onetime_address - Ko
     * outparam: view_tag_out - vt
     */
    virtual void make_internal_view_tag(const input_context_t &input_context,
        const crypto::public_key &onetime_address,
        view_tag_t &view_tag_out) const = 0;

    /**
     * brief: make_internal_sender_receiver_secret - make internal sender-receiver secret, given non-secret data
     *   s^ctx_sr = H_32(s_sr, D_e, input_context)
     * param: enote_ephemeral_pubkey - D_e
     * param: input_context - input_context
     * outparam: s_sender_receiver_out - s^ctx_sr
     */
    virtual void make_internal_sender_receiver_secret(const mx25519_pubkey &enote_ephemeral_pubkey,
        const input_context_t &input_context,
        crypto::hash &s_sender_receiver_out) const = 0;

    virtual ~view_balance_secret_device() = default;
};

struct generate_address_secret_device
{
    /**
    * brief: make_index_extension_generator - make carrot index extension generator s^j_gen
    *   s^j_gen = H_32[s_ga](j_major, j_minor)
    * param: major_index - j_major
    * param: minor_index - j_minor
    * outparam: address_generator_out - s^j_gen
    */
    virtual void make_index_extension_generator(const std::uint32_t major_index,
        const std::uint32_t minor_index,
        crypto::secret_key &address_generator_out) const = 0;

    virtual ~generate_address_secret_device() = default;
};

} //namespace carrot
