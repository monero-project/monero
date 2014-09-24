// Copyright (c) 2014, The Monero Project
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
#include <fstream>
#include "mnemonics/electrum-words.h"
#include <stdexcept>

namespace
{
  int num_words = 0; 
  std::map<std::string,uint32_t> words_map;
  std::vector<std::string> words_array;

  const std::string WORD_LISTS_DIRECTORY = "wordlists";
  const std::string LANGUAGES_DIRECTORY = "languages";
  const std::string OLD_WORD_FILE = "old-word-list";

  bool is_first_use()
  {
    return num_words == 0 ? true : false;
  }

  void create_data_structures(const std::string &word_file)
  {
    std::ifstream input_stream;
    input_stream.open(word_file.c_str(), std::ifstream::in);

    if (!input_stream)
      throw std::runtime_error(std::string("Word list file couldn't be opened."));

    std::string word;
    while (input_stream >> word)
    {
      words_array.push_back(word);
      words_map[word] = num_words;
      num_words++;
    }
    input_stream.close();
  }
}

namespace crypto
{
  namespace ElectrumWords
  {

    void init(const std::string &language, bool old_word_list = false)
    {
      if (old_word_list)
      {
        create_data_structures(WORD_LISTS_DIRECTORY + '/' + OLD_WORD_FILE);
      }
      else
      {
        create_data_structures(WORD_LISTS_DIRECTORY + '/' + LANGUAGES_DIRECTORY + '/' + language);
      }
    }

    /* convert words to bytes, 3 words -> 4 bytes
     * returns:
     *    false if not a multiple of 3 words, or if a words is not in the
     *    words list
     *
     *    true otherwise
     */
    bool words_to_bytes(const std::string& words, crypto::secret_key& dst)
    {
      if (is_first_use())
      {
        init("", true);
      }
      int n = num_words;

      std::vector<std::string> wlist;

      boost::split(wlist, words, boost::is_any_of(" "));

      // error on non-compliant word list
      if (wlist.size() != 12 && wlist.size() != 24) return false;

      for (unsigned int i=0; i < wlist.size() / 3; i++)
      {
        uint32_t val;
        uint32_t w1, w2, w3;

        // verify all three words exist in the word list
        if (words_map.count(wlist[i*3]) == 0 ||
            words_map.count(wlist[i*3 + 1]) == 0 ||
            words_map.count(wlist[i*3 + 2]) == 0)
        {
          return false;
        }

        w1 = words_map.at(wlist[i*3]);
        w2 = words_map.at(wlist[i*3 + 1]);
        w3 = words_map.at(wlist[i*3 + 2]);

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
      if (is_first_use())
      {
        init("", true);
      }
      int n = num_words;

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

        words += words_array[w1];
        words += ' ';
        words += words_array[w2];
        words += ' ';
        words += words_array[w3];
      }
      return false;
    }

  }  // namespace ElectrumWords

}  // namespace crypto
