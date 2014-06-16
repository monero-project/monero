/*
 * This file and its header file are for translating Electrum-style word lists
 * into their equivalent byte representations for cross-compatibility with
 * that method of "backing up" one's wallet keys.
 */

#include <string>
#include <cassert>
#include <map>
#include <cstdint>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "crypto/crypto.h"  // for declaration of crypto::secret_key

#include "crypto/electrum-words.h"

namespace crypto
{
  namespace ElectrumWords
  {

    /* convert words to bytes, 3 words -> 4 bytes
     * returns:
     *    false if not a multiple of 3 words, or if a words is not in the
     *    words list
     *
     *    true otherwise
     */
    bool words_to_bytes(const std::string& words, crypto::secret_key& dst)
    {
      int n = NUMWORDS; // hardcoded because this is what electrum uses

      std::vector<std::string> wlist;

      boost::split(wlist, words, boost::is_any_of(" "));

      // error on non-compliant word list
      if (wlist.size() != 12 && wlist.size() != 24) return false;

      for (unsigned int i=0; i < wlist.size() / 3; i++)
      {
        uint32_t val;
        uint32_t w1, w2, w3;

        // verify all three words exist in the word list
        if (wordsMap.count(wlist[i*3]) == 0 ||
            wordsMap.count(wlist[i*3 + 1]) == 0 ||
            wordsMap.count(wlist[i*3 + 2]) == 0)
        {
          return false;
        }

        w1 = wordsMap.at(wlist[i*3]);
        w2 = wordsMap.at(wlist[i*3 + 1]);
        w3 = wordsMap.at(wlist[i*3 + 2]);

        val = w1 + n * (((n - w1) + w2) % n) + n * n * (((n - w2) + w3) % n);

        if (!(val % n == w1)) return false;

        memcpy(dst.data + i * 4, &val, 4);  // copy 4 bytes to position
      }

      std::string wlist_copy = words;
      if (wlist.size() == 12)
      {
        memcpy(dst.data, dst.data + 16, 16);  // if electrum 12-word seed, duplicate
        wlist_copy += ' ';
        wlist_copy += words;
      }

      return true;
    }

    /* convert bytes to words, 4 bytes-> 3 words
     * returns:
     *    false if wrong number of bytes (shouldn't be possible)
     *    true otherwise
     */
    bool bytes_to_words(const crypto::secret_key& src, std::string& words)
    {
      int n = NUMWORDS; // hardcoded because this is what electrum uses

      if (sizeof(src.data) % 4 != 0) return false;

      // 8 bytes -> 3 words.  8 digits base 16 -> 3 digits base 1626
      for (unsigned int i=0; i < sizeof(src.data)/4; i++, words += ' ')
      {
        uint32_t w1, w2, w3;
        
        uint32_t val;

        memcpy(&val, (src.data) + (i * 4), 4);

        w1 = val % n;
        w2 = ((val / n) + w1) % n;
        w3 = (((val / n) / n) + w2) % n;

        words += wordsArray[w1];
        words += ' ';
        words += wordsArray[w2];
        words += ' ';
        words += wordsArray[w3];
      }
      return false;
    }

  }  // namespace ElectrumWords

}  // namespace crypto
