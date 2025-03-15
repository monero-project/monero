// Copyright (c) 2022, The Monero Project
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

#include "cryptonote_basic.h"

#include "cryptonote_basic_impl.h"
#include "serialization/wire.h"
#include "serialization/wire/adapted/vector.h"
#include "serialization/wire/wrapper/array.h"
#include "serialization/wire/wrapper/variant.h"
#include "serialization/wire/wrappers_impl.h"

namespace cryptonote
{
  namespace
  {
    template<typename F, typename T>
    void txin_gen_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(height));
    }

    template<typename F, typename T>
    void txin_to_script_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(prev), WIRE_FIELD(prevout), WIRE_FIELD(sigset));
    }

    template<typename F, typename T>
    void txin_to_scripthash_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(prev), WIRE_FIELD(prevout), WIRE_FIELD(script), WIRE_FIELD(sigset));
    }

    template<typename F, typename T>
    void txin_to_key_map(F& format, T& self)
    {
      using max_ring_size = wire::max_element_count<5000>;
      wire::object(format,
        WIRE_FIELD(amount),
        wire::field("key_offsets", wire::array<max_ring_size>(std::ref(self.key_offsets))),
        wire::field("key_image", std::ref(self.k_image))
      );
    }

    template<typename F, typename T>
    void txin_v_map(F& format, T& self)
    {
      auto variant = wire::variant(std::ref(self));
      wire::object(format,
        WIRE_OPTION("gen",           txin_gen,           variant),
        WIRE_OPTION("to_script",     txin_to_script,     variant),
        WIRE_OPTION("to_scripthash", txin_to_scripthash, variant),
        WIRE_OPTION("to_key",        txin_to_key,        variant)
      );
    }
  } // anonymous
  WIRE_DEFINE_OBJECT(txin_gen, txin_gen_map)
  WIRE_DEFINE_OBJECT(txin_to_script, txin_to_script_map)
  WIRE_DEFINE_OBJECT(txin_to_scripthash, txin_to_scripthash_map)
  WIRE_DEFINE_OBJECT(txin_to_key, txin_to_key_map)
  WIRE_DEFINE_OBJECT(txin_v, txin_v_map)

  namespace
  {
    template<typename F, typename T>
    void txout_to_script_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(keys), WIRE_FIELD(script));
    }

    template<typename F, typename T>
    void txout_to_scripthash_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(hash));
    }

    template<typename F, typename T>
    void txout_to_key_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(key));
    }

    template<typename F, typename T>
    void txout_to_tagged_key_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(key), WIRE_FIELD(view_tag));
    }

    template<typename F, typename T>
    void tx_out_map(F& format, T& self)
    {
      auto variant = wire::variant(std::ref(self.target));
      wire::object(format,
        WIRE_FIELD(amount),
        WIRE_OPTION("to_key",        txout_to_key,        variant),
        WIRE_OPTION("to_tagged_key", txout_to_tagged_key, variant),
        WIRE_OPTION("to_script",     txout_to_script,     variant),
        WIRE_OPTION("to_scripthash", txout_to_scripthash, variant)
      );
    }
  } // anonymous
  WIRE_DEFINE_OBJECT(txout_to_script, txout_to_script_map)
  WIRE_DEFINE_OBJECT(txout_to_scripthash, txout_to_scripthash_map)
  WIRE_DEFINE_OBJECT(txout_to_key, txout_to_key_map)
  WIRE_DEFINE_OBJECT(txout_to_tagged_key, txout_to_tagged_key_map)
  WIRE_DEFINE_OBJECT(tx_out, tx_out_map)

  namespace
  {
    template<typename F, typename T>
    void tx_map(F& format, T& self)
    {
      // make all arrays required for ZMQ backwards compatability
      using signature_min = wire::min_element_sizeof<crypto::signature>;
      wire::object(format,
        WIRE_FIELD(version),
        WIRE_FIELD(unlock_time),
        wire::field("inputs", wire::array<max_inputs_per_tx>(std::ref(self.vin))),
        wire::field("outputs", wire::array<max_outputs_per_tx>(std::ref(self.vout))),
        WIRE_FIELD(extra),
        wire::optional_field("signatures", wire::array<max_inputs_per_tx>(wire::array<signature_min>(std::ref(self.signatures)))),
        wire::field("ringct", prune_wrapper(std::ref(self.rct_signatures), self.pruned))
      );
    }

    template<typename F, typename T>
    void block_map(F& format, T& self)
    {
      // make all arrays required for ZMQ backwards compatability
      wire::object(format,
        WIRE_FIELD(major_version),
        WIRE_FIELD(minor_version),
        WIRE_FIELD(timestamp),
        WIRE_FIELD(prev_id),
        WIRE_FIELD(nonce),
        WIRE_FIELD(miner_tx),
        wire::field("tx_hashes", wire::array<wire::min_element_sizeof<crypto::hash>>(std::ref(self.tx_hashes)))
      );
    }
  } // anonymous

  void read_bytes(wire::reader& source, transaction& dest)
  {
    dest.invalidate_hashes();
    tx_map(source, dest);

    const auto& rsig = dest.rct_signatures;
    if (!cryptonote::is_coinbase(dest) && rsig.p.bulletproofs.empty() && rsig.p.bulletproofs_plus.empty() && rsig.p.rangeSigs.empty() && rsig.p.MGs.empty() && rsig.get_pseudo_outs().empty() && dest.signatures.empty())
      dest.pruned = true;
    else
      dest.pruned = false;
  }
  void write_bytes(wire::writer& dest, const transaction& source)
  { tx_map(dest, source); }

  void read_bytes(wire::reader& source, block& dest)
  {
    dest.invalidate_hashes();
    block_map(source, dest);
  }
  void write_bytes(wire::writer& dest, const block& source)
  { block_map(dest, source); }
} // cryptonote
