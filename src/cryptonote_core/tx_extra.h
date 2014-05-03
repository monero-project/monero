// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#define TX_EXTRA_PADDING_MAX_COUNT          255
#define TX_EXTRA_NONCE_MAX_COUNT            255

#define TX_EXTRA_TAG_PADDING                0x00
#define TX_EXTRA_TAG_PUBKEY                 0x01
#define TX_EXTRA_NONCE                      0x02

#define TX_EXTRA_NONCE_PAYMENT_ID           0x00

namespace cryptonote
{
  struct tx_extra_padding
  {
    size_t size;

    // load
    template <template <bool> class Archive>
    bool do_serialize(Archive<false>& ar)
    {
      // size - 1 - because of variant tag
      for (size = 1; size <= TX_EXTRA_PADDING_MAX_COUNT; ++size)
      {
        std::ios_base::iostate state = ar.stream().rdstate();
        bool eof = EOF == ar.stream().peek();
        ar.stream().clear(state);

        if (eof)
          break;

        uint8_t zero;
        if (!::do_serialize(ar, zero))
          return false;

        if (0 != zero)
          return false;
      }

      return size <= TX_EXTRA_PADDING_MAX_COUNT;
    }

    // store
    template <template <bool> class Archive>
    bool do_serialize(Archive<true>& ar)
    {
      if(TX_EXTRA_PADDING_MAX_COUNT < size)
        return false;

      // i = 1 - because of variant tag
      for (size_t i = 1; i < size; ++i)
      {
        uint8_t zero = 0;
        if (!::do_serialize(ar, zero))
          return false;
      }
      return true;
    }
  };

  struct tx_extra_pub_key
  {
    crypto::public_key pub_key;

    BEGIN_SERIALIZE()
      FIELD(pub_key)
    END_SERIALIZE()
  };

  struct tx_extra_nonce
  {
    std::string nonce;

    BEGIN_SERIALIZE()
      FIELD(nonce)
      if(TX_EXTRA_NONCE_MAX_COUNT < nonce.size()) return false;
    END_SERIALIZE()
  };

  // tx_extra_field format, except tx_extra_padding and tx_extra_pub_key:
  //   varint tag;
  //   varint size;
  //   varint data[];
  typedef boost::variant<tx_extra_padding, tx_extra_pub_key, tx_extra_nonce> tx_extra_field;
}

VARIANT_TAG(binary_archive, cryptonote::tx_extra_padding, TX_EXTRA_TAG_PADDING);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_pub_key, TX_EXTRA_TAG_PUBKEY);
VARIANT_TAG(binary_archive, cryptonote::tx_extra_nonce, TX_EXTRA_NONCE);
