/*
 * This file and its header file are for translating Electrum-style word lists
 * into their equivalent byte representations for cross-compatibility with
 * that method of "backing up" one's wallet keys.
 */

#include <string>
#include <map>
#include "crypto/crypto.h"  // for declaration of crypto::secret_key

#include "crypto/electrum-words.h"

namespace crypto
{
  namespace ElectrumWords
  {

    bool words_to_bytes(const std::string& words, crypto::secret_key& dst)
    {
      return false;
    }
    bool bytes_to_words(const crypto::secret_key& src, std::string& words)
    {
      return false;
    }

  }  // namespace ElectrumWords

}  // namespace crypto
