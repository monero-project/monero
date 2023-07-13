// Copyright (c) 2023, The Monero Project
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

#include "noise.h"

#include <boost/endian/arithmetic.hpp>
#include <boost/optional/optional.hpp>
#include <cstring>
#include <limits>
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_auth_hmacsha256.h>
#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_kx.h>
#include <sodium/crypto_scalarmult.h>
#include <sodium/crypto_stream_chacha20.h>
#include <stdexcept>
#include <type_traits>

#include "byte_stream.h"
#include "net/levin_base.h"
#include "span.h"

namespace epee { namespace noise
{
  namespace
  {
    template<std::size_t N>
    using constant = std::integral_constant<std::size_t, N>;

    template<std::size_t N>
    using buffer = std::array<std::uint8_t, N>;

    struct v0
    {
      static constexpr const char* protocol_name() noexcept
      { return "MONERO_P2P_V0_NOISE_NN_25519_CHACHA20-POLY1305_SHA256"; }

      using hashlen = constant<32>;  //!< sha256
      using blocklen = constant<64>;//!< sha256
      using digest = buffer<hashlen::value>;//!< Output of sha256
      using nonce = boost::endian::little_uint64_t;
      using noncelen = constant<sizeof(nonce)>;  //!< chacha20 nonce
      using keylen = constant<32>;   //!< chacha20 256-bit key
      using key = buffer<keylen::value>; //!< chacha20
      using dhpublen = constant<32>;//!< x25519
      using dhscalarlen = constant<32>;//!< x25519
      using dhscalar = buffer<dhscalarlen::value>;
      using dhpub = buffer<dhpublen::value>; //!< x25519
      using taglen = constant<crypto_aead_chacha20poly1305_ABYTES>;

      static dhpub scalarmult(const dhscalar& sec, const dhpub& pub)
      {
        if (boost::string_ref{crypto_scalarmult_PRIMITIVE} != "curve25519")
          throw std::runtime_error{"Unexpected change in libsodium key exchange library"};

        dhpub out{};
        if (crypto_scalarmult(out.data(), sec.data(), pub.data()) != 0)
          throw std::runtime_error{"failed scalarmult"};
        return out;
      }

      static digest hash(epee::span<const std::uint8_t> data)
      {
        digest out{};
        if (crypto_hash_sha256(out.data(), data.data(), data.size()) != 0)
          throw std::runtime_error{"failed sha256 hashing"};
        return out;
      }

      static digest hmac(epee::span<const std::uint8_t> key, epee::span<const std::uint8_t> data)
      {
        digest out{};
        if (key.size() != crypto_auth_hmacsha256_KEYBYTES && crypto_auth_hmacsha256(out.data(), data.data(), data.size(), key.data()) != 0)
          throw std::runtime_error{"failed hmac-sha256 hashing"};
        return out;
      }

      static_assert(std::numeric_limits<std::size_t>::max() <= std::numeric_limits<unsigned long long>::max(), "invalid unsigned long long");
  
      static void peek(epee::span<std::uint8_t> out, epee::span<const std::uint8_t> in, const key& k, const nonce n)
      {
        if (crypto_stream_chacha20_xor(out.data(), in.data(), std::min(out.size(), in.size()), n.data(), k.data()) == 0)
          throw std::runtime_error{"crypto_stream_xchacha failed"};
      }

      static std::size_t encrypt(epee::span<std::uint8_t> out, const key& k, const nonce n, epee::span<const std::uint8_t> in, const digest& ad)
      {
        unsigned long long clen = out.size();
        if (crypto_aead_chacha20poly1305_encrypt(out.data(), std::addressof(clen), in.data(), in.size(), ad.data(), ad.size(), nullptr, n.data(), k.data()) != 0)
          throw std::runtime_error{"crypto_aead_chacha20poly1305_encrypt failed"};
        return clen;
      }

      //! \return Actual length of decrypted data (poly1305 tag omitted).
      static std::size_t decrypt(epee::span<std::uint8_t> out, const key& k, const nonce n, epee::span<const std::uint8_t> in, const digest& ad)
      {
        unsigned long long mlen = out.size();
        if (crypto_aead_chacha20poly1305_decrypt(out.data(), std::addressof(mlen), nullptr, in.data(), in.size(), ad.data(), ad.size(), n.data(), k.data()) != 0)
          throw std::runtime_error{"crypto_aead_chacha20poly1305_decrypt failed"};
        return mlen;
      }
    };

    template<typename V>
    class cipher_state
    {
      using digest = typename V::digest;
      using key = typename V::key;
      using noncelen = typename V::noncelen;
      using taglen = typename V::taglen;

      std::uint64_t n; //!< nonce
      boost::optional<key> k;
      static_assert(sizeof(n) == noncelen::value, "unexpected nonce size");

    public:
      cipher_state()
       : n(0), k()
      {}

      cipher_state(const cipher_state&) noexcept = default;
      cipher_state(cipher_state&&) noexcept = default;
      cipher_state& operator=(const cipher_state&) noexcept = default;
      cipher_state& operator=(cipher_state&&) noexcept = default;

      void initialize_key(const key& newk)
      {
        k = newk;
        n = 0;
      }
      void rekey(const key& newk) { k = newk; }

      void peek(epee::span<std::uint8_t> out, epee::span<const std::uint8_t> in)
      {
        V::peek(out, in, *k, n);
      }

      //! \return Actual length of decrypted data (poly1305 tag removed).
      std::size_t decrypt(epee::span<std::uint8_t> out, epee::span<const std::uint8_t> in, const digest& ad)
      {
        if (k)
          return V::decrypt(out, *k, n++, in, ad);
        else if (in.size() <= out.size())
          std::memcpy(out.data(), in.data(), in.size());
        else
          throw std::logic_error{"Invalid output buffer size"};
        return in.size();
      }

      //! \return Actual length of encrypted data.
      std::size_t encrypt(epee::span<std::uint8_t> out, epee::span<const std::uint8_t> in, const digest& ad)
      {
        if (n == std::numeric_limits<std::uint64_t>::max())
          throw std::runtime_error{"Exceeded nonces in encrypt"};
        if (k)
          return V::encrypt(out, *k, n++, in, ad);
        else if (in.size() <= out.size())
          std::memcpy(out.data(), in.data(), in.size());
        else
          throw std::logic_error{"Invalid output buffer size"};
        return in.size();
      }
    };
  
    template<typename V>
    class symmetric_state
    {
      using key = typename V::key;
      using hashlen = typename V::hashlen;
      using digest = typename V::digest;
      using taglen = typename V::taglen;
      
      cipher_state<V> cipher;
      digest hash;        //!< `h` value in spec
      digest chaining_key;//!< `ck` value in spec
      
    public:
      explicit symmetric_state() noexcept
        : cipher{}, hash{}, chaining_key{}
      {
        const boost::string_ref protocol_name{V::protocol_name()};
        if (protocol_name.size() <= hash.size())
          std::memcpy(hash.data(), protocol_name.data(), protocol_name.size());
        else
          hash = V::hash({reinterpret_cast<const std::uint8_t*>(protocol_name.data()), protocol_name.size()});
      }

      symmetric_state(const symmetric_state&) noexcept = default;
      symmetric_state(symmetric_state&&) noexcept = default;
      symmetric_state& operator=(const symmetric_state&) noexcept = default;
      symmetric_state& operator=(symmetric_state&&) noexcept = default;

      const digest& get_handshake_hash() const noexcept { return hash; }

      void peek(epee::span<std::uint8_t> out, epee::span<const std::uint8_t> in)
      {
        cipher.peek(out, in);
      }

      std::array<digest, 2> hkdf_2(epee::span<const std::uint8_t> input)
      {
        const std::uint8_t byte = 0x01;
        const digest temp_key = V::hmac(epee::to_span(chaining_key), input);
        const digest output1 = V::hmac(epee::to_span(temp_key), {std::addressof(byte), 1});
        
        buffer<hashlen::value + 1> temp_data{};
        std::memcpy(temp_data.data(), output1.data(), output1.size());
        temp_data[hashlen::value] = 0x02;

        const digest output2 = V::hmac(epee::to_span(temp_key), epee::to_span(temp_data));

        return {{output1, output2}};
      }

      std::array<digest, 3> hkdf_3(epee::span<const std::uint8_t> input)
      {
        const digest temp_key = V::hmac(epee::to_span(chaining_key), input);
        const std::uint8_t byte = 0x01;
        const digest output1 = V::hmac(epee::to_span(temp_key), {std::addressof(byte), 1});
        
        buffer<hashlen::value + 1> next{};
        std::memcpy(next.data(), output1.data(), output1.size());
        next[hashlen::value] = 0x02;

        const digest output2 = V::hmac(epee::to_span(temp_key), epee::to_span(next));

        std::memcpy(next.data(), output2.data(), output2.size());
        next[hashlen::value] = 0x03;

        const digest output3 = V::hmac(epee::to_span(temp_key), epee::to_span(next));

        return {{output1, output2, output3}};
      }

      void mix_key(epee::span<const std::uint8_t> input)
      {
        const auto temp = hkdf_2(input);
        hash = temp[0];
        key temp_k{};
        static_assert(temp_k.size() <= temp[1].size(), "bad hash size");
        std::memcpy(temp_k.data(), temp[1].data(), temp_k.size());
        cipher.initialize_key(temp_k);
      }

      void mix_hash(epee::span<const std::uint8_t> in)
      {
        if (std::numeric_limits<std::size_t>::max() - hash.size() < in.size())
          throw std::runtime_error{"overflow in mix_hash"};
        byte_stream bytes{};
        bytes.resize(hash.size() + in.size());
        std::memcpy(bytes.data(), hash.data(), hash.size());
        std::memcpy(bytes.data() + hash.size(), in.data(), in.size());
        hash = V::hash(epee::to_span(bytes));
      }

      void mix_key_and_hash(epee::span<const std::uint64_t> in)
      {
        const auto temp = hkdf_3(in);
        chaining_key = temp[0];
        mix_hash(epee::to_span(temp[1]));
        
        key temp_k;
        static_assert(temp.size() <= temp[1].size(), "bad hash size");
        std::memcpy(temp_k.data(), temp[1].data(), temp_k.size());
        cipher.initialize_key(temp_k);
      }

      byte_slice encrypt_and_hash(epee::span<const std::uint8_t> in)
      {
        if (std::numeric_limits<std::size_t>::max() - taglen() < in.size())
          throw std::runtime_error{"Overflowed max size_t in encrypt_and_hash"};
        byte_stream out{};
        out.resize(in.size() + taglen());
        out.resize(cipher.encrypt(epee::to_mut_span(out), in, hash));
        mix_hash(epee::to_span(out));
        return byte_slice{std::move(out)};
      }

      void decrypt_and_hash(span<std::uint8_t> out, span<const std::uint8_t> in)
      {
        cipher.decrypt(out, in, hash);
        mix_hash(in);
      }

      std::array<symmetric_state<V>, 2> split()
      {
        key newk{};
        std::array<symmetric_state<V>, 2> out;
        const auto temp = hkdf_2(nullptr);

        static_assert(newk.size() <= temp[1].size(), "bad hash size");
        std::memcpy(newk.data(), temp[0].data(), newk.size());
        out[0].cipher.initialize_key(newk);

        std::memcpy(newk.data(), temp[1].data(), newk.size());
        out[1].cipher.initialize_key(newk);

        return out;
      }
    };
  } // anonymous


  //! Implement NN only right now
  struct handshake_state
  {
    using version = v0;
    
    struct complete
    {
      symmetric_state<version> incoming;
      symmetric_state<version> outgoing;
      net_utils::buffer in_buffer;
      std::vector<byte_slice> out_queue;
    };

    symmetric_state<version> state;
    std::vector<byte_slice> out_queue; //!< Requests to send data before handshake complete
    net_utils::buffer in_buffer;
    version::dhscalar e_secret;
    version::dhpub e_public;
    boost::optional<version::dhpub> re;
    const bool initiator;

    handshake_state(net_utils::i_service_endpoint& peer, const bool initiator)
      : state{}, initiator(initiator)
    {
      if (boost::string_ref{crypto_kx_PRIMITIVE} != "x25519blake2b")
        throw std::runtime_error{"libsodium changed its base crypto_kx_keypair function"};
      crypto_kx_keypair(e_public.data(), e_secret.data());
      state.mix_hash(nullptr);

      //! \TODO Obfuscation of first bytes? Less useful without static pubkey
      if (initiator)
      {
        if (!peer.do_send(byte_slice{{to_span(e_public)}}))
          throw std::runtime_error{"Failed to send initiator ephermal public key"};
        state.mix_hash(to_span(e_public));
      }
    }

    boost::optional<complete> read_message(net_utils::i_service_endpoint& peer, void const* const data, std::size_t length)
    {
      in_buffer.append(data, length);
      if (in_buffer.size() < version::dhpublen())
        return boost::none;

      re.emplace();
      std::memcpy(re->data(), in_buffer.span(version::dhpublen()).data(), re->size());
      in_buffer.erase(version::dhpublen());
      state.mix_hash(to_span(*re));

      if (!initiator)
      {
        if (!peer.do_send(byte_slice{{to_span(e_public)}}))
          throw std::runtime_error{"Failed to send responder ephermal public key"};
        state.mix_hash(to_span(e_public));
      }

      const version::dhpub ee = version::scalarmult(e_secret, *re);
      state.mix_key(to_span(ee));
      auto states = state.split();
      if (initiator)
        return complete{std::move(states[0]), std::move(states[1]), std::move(in_buffer), std::move(out_queue)};
      else
        return complete{std::move(states[1]), std::move(states[0]), std::move(in_buffer), std::move(out_queue)};
        return complete{std::move(states[1]), std::move(states[0]), std::move(in_buffer), std::move(out_queue)};
    }
  };

  struct stream
  {
    using version = v0;

    symmetric_state<version> incoming;
    symmetric_state<version> outgoing;
    net_utils::buffer in_buffer;

    stream(handshake_state::complete&& initial)
      : incoming(std::move(initial.incoming)),
        outgoing(std::move(initial.outgoing)),
        in_buffer(std::move(initial.in_buffer))
    {}

    std::pair<state, net_utils::buffer> process(void const* const data, std::size_t length)
    {
      std::vector<std::uint8_t> plaintext;
      in_buffer.append(data, length);
      while (sizeof(levin::bucket_head2) <= in_buffer.size())
      {
        levin::bucket_head2 header{};
        incoming.peek(as_mut_byte_span(header), in_buffer.span(sizeof(header)));
        if (in_buffer.size() < header.m_cb)
        {
          if (LEVIN_DEFAULT_MAX_PACKET_SIZE < header.m_cb)
            return {state::error, net_utils::buffer{}};
          break;
        }

        const std::size_t start = plaintext.size();
        if (plaintext.max_size() - start < sizeof(header))
          return {state::error, net_utils::buffer{}};
        if (plaintext.max_size() - start - sizeof(header) < header.m_cb)
          return {state::error, net_utils::buffer{}};
        plaintext.resize(start + sizeof(header) + header.m_cb);
        incoming.decrypt_and_hash({plaintext.data() + start, header.m_cb}, in_buffer.carve(sizeof(header) + header.m_cb));
      }

      const auto rc = plaintext.empty() ?
        state::buffering : state::decrypted;
      
      return {rc, net_utils::buffer{std::move(plaintext)}};
    }

    bool send(net_utils::i_service_endpoint& peer, byte_slice plaintext)
    {
      return peer.do_send(outgoing.encrypt_and_hash(to_span(plaintext)));
    }
  };

  protocol::protocol(net_utils::i_service_endpoint& peer, const bool initiator)
    : handshake_{new handshake_state(peer, initiator)}, stream_{nullptr}
  {}

  protocol::~protocol()
  {}

  bool protocol::encrypt(net_utils::i_service_endpoint& peer, byte_slice plaintext)
  {
    if (handshake_)
    {
      // cannot encrypt data until handshake is complete, hold it temporarily
      handshake_->out_queue.push_back(std::move(plaintext));
      return true;
    }
    if (!stream_)
      throw std::runtime_error{"protocol encryption in invalid state"};
    return stream_->send(peer, std::move(plaintext));
  }

  std::pair<state, net_utils::buffer> protocol::decrypt(net_utils::i_service_endpoint& peer, void const* const data, const std::size_t length)
  {
    if (handshake_)
    {
      auto complete = handshake_->read_message(peer, data, length);
      if (!complete)
        return {state::buffering, net_utils::buffer{}};

      std::vector<byte_slice> out_queue = std::move(complete->out_queue);
      stream_.reset(new stream{std::move(*complete)});
      handshake_.reset();
      
      for (auto& message : out_queue)
        stream_->send(peer, std::move(message));

      return stream_->process(nullptr, 0); // process anything left in buffer
    }
    else if (stream_)
      return stream_->process(data, length);
    throw std::runtime_error{"protocol decryption in invalid state"};
  }
}} // epee // noise
