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

/*!
 * \file electrum-words.cpp
 * 
 * \brief Mnemonic seed generation and wallet restoration from them.
 * 
 * This file and its header file are for translating Electrum-style word lists
 * into their equivalent byte representations for cross-compatibility with
 * that method of "backing up" one's wallet keys.
 */

#include <string>
#include <cassert>
#include <map>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include "crypto/crypto.h"  // for declaration of crypto::secret_key
#include <fstream>
#include "mnemonics/electrum-words.h"
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/crc.hpp>
#include <boost/algorithm/string/join.hpp>

#include "english.h"
#include "spanish.h"
#include "portuguese.h"
#include "japanese.h"
#include "old_english.h"
#include "language_base.h"
#include "singleton.h"

namespace
{
  const int seed_length = 24;

  /*!
   * \brief Finds the word list that contains the seed words and puts the indices
   *        where matches occured in matched_indices.
   * \param  seed            List of words to match.
   * \param  has_checksum    If word list passed checksum test, we need to only do a prefix check.
   * \param  matched_indices The indices where the seed words were found are added to this.
   * \return                 true if all the words were present in some language false if not.
   */
  bool find_seed_language(const std::vector<std::string> &seed,
    bool has_checksum, std::vector<uint32_t> &matched_indices, uint32_t &word_list_length,
    std::string &language_name)
  {
    // If there's a new language added, add an instance of it here.
    std::vector<Language::Base*> language_instances({
      Language::Singleton<Language::English>::instance(),
      Language::Singleton<Language::Spanish>::instance(),
      Language::Singleton<Language::Portuguese>::instance(),
      Language::Singleton<Language::Japanese>::instance(),
      Language::Singleton<Language::OldEnglish>::instance()
    });
    // To hold trimmed seed words in case of a checksum being present.
    std::vector<std::string> trimmed_seed;
    if (has_checksum)
    {
      // If it had a checksum, we'll just compare the unique prefix
      // So we create a list of trimmed seed words
      for (std::vector<std::string>::const_iterator it = seed.begin(); it != seed.end(); it++)
      {
        trimmed_seed.push_back(it->length() > Language::unique_prefix_length ?
          it->substr(0, Language::unique_prefix_length) : *it);
      }
    }
    std::unordered_map<std::string, uint32_t> word_map;
    std::unordered_map<std::string, uint32_t> trimmed_word_map;
    // Iterate through all the languages and find a match
    for (std::vector<Language::Base*>::iterator it1 = language_instances.begin();
      it1 != language_instances.end(); it1++)
    {
      word_map = (*it1)->get_word_map();
      trimmed_word_map = (*it1)->get_trimmed_word_map();
      // To iterate through seed words
      std::vector<std::string>::const_iterator it2;
      // To iterate through trimmed seed words
      std::vector<std::string>::iterator it3;
      bool full_match = true;

      // Iterate through all the words and see if they're all present
      for (it2 = seed.begin(), it3 = trimmed_seed.begin();
        it2 != seed.end(); it2++, it3++)
      {
        if (has_checksum)
        {
          // Use the trimmed words and map
          if (trimmed_word_map.count(*it3) == 0)
          {
            full_match = false;
            break;
          }
          matched_indices.push_back(trimmed_word_map[*it3]);
        }
        else
        {
          if (word_map.count(*it2) == 0)
          {
            full_match = false;
            break;
          }
          matched_indices.push_back(word_map[*it2]);
        }
      }
      if (full_match)
      {
        word_list_length = (*it1)->get_word_list().size();
        language_name = (*it1)->get_language_name();
        return true;
      }
      // Some didn't match. Clear the index array.
      matched_indices.clear();
    }
    return false;
  }

  /*!
   * \brief Creates a checksum index in the word list array on the list of words.
   * \param  word_list Vector of words
   * \return           Checksum index
   */
  uint32_t create_checksum_index(const std::vector<std::string> &word_list)
  {
    std::string trimmed_words = "";

    for (std::vector<std::string>::const_iterator it = word_list.begin(); it != word_list.end(); it++)
    {
      if (it->length() > 4)
      {
        trimmed_words += it->substr(0, Language::unique_prefix_length);
      }
      else
      {
        trimmed_words += *it;
      }
    }
    boost::crc_32_type result;
    result.process_bytes(trimmed_words.data(), trimmed_words.length());
    return result.checksum() % seed_length;
  }

  /*!
   * \brief Does the checksum test on the seed passed.
   * \param seed    Vector of seed words
   * \return        True if the test passed false if not.
   */
  bool checksum_test(std::vector<std::string> seed)
  {
    // The last word is the checksum.
    std::string last_word = seed.back();
    seed.pop_back();

    std::string checksum = seed[create_checksum_index(seed)];

    std::string trimmed_checksum = checksum.length() > 4 ? checksum.substr(0, Language::unique_prefix_length) :
      checksum;
    std::string trimmed_last_word = checksum.length() > 4 ? last_word.substr(0, Language::unique_prefix_length) :
      last_word;
    return trimmed_checksum == trimmed_last_word;
  }
}

/*!
 * \namespace crypto
 * 
 * \brief crypto namespace.
 */
namespace crypto
{
  /*!
   * \namespace crypto::ElectrumWords
   * 
   * \brief Mnemonic seed word generation and wallet restoration helper functions.
   */
  namespace ElectrumWords
  {
    /*!
     * \brief Converts seed words to bytes (secret key).
     * \param  words           String containing the words separated by spaces.
     * \param  dst             To put the secret key restored from the words.
     * \param  language_name   Language of the seed as found gets written here.
     * \return                 false if not a multiple of 3 words, or if word is not in the words list
     */
    bool words_to_bytes(const std::string& words, crypto::secret_key& dst,
      std::string &language_name)
    {
      std::vector<std::string> seed;

      boost::split(seed, words, boost::is_any_of(" "));

      // error on non-compliant word list
      if (seed.size() != seed_length/2 && seed.size() != seed_length &&
        seed.size() != seed_length + 1)
      {
        return false;
      }

      // If it is seed with a checksum.
      bool has_checksum = seed.size() == (seed_length + 1);
      if (has_checksum)
      {
        if (!checksum_test(seed))
        {
          // Checksum fail
          return false;
        }
        seed.pop_back();
      }
      
      std::vector<uint32_t> matched_indices;
      uint32_t word_list_length = 0;
      if (!find_seed_language(seed, has_checksum, matched_indices, word_list_length, language_name))
      {
        return false;
      }

      for (unsigned int i=0; i < seed.size() / 3; i++)
      {
        uint32_t val;
        uint32_t w1, w2, w3;
        w1 = matched_indices[i*3];
        w2 = matched_indices[i*3 + 1];
        w3 = matched_indices[i*3 + 2];

        val = w1 + word_list_length * (((word_list_length - w1) + w2) % word_list_length) +
          word_list_length * word_list_length * (((word_list_length - w2) + w3) % word_list_length);

        if (!(val % word_list_length == w1)) return false;

        memcpy(dst.data + i * 4, &val, 4);  // copy 4 bytes to position
      }

      std::string wlist_copy = words;
      if (seed.size() == seed_length/2)
      {
        memcpy(dst.data, dst.data + 16, 16);  // if electrum 12-word seed, duplicate
        wlist_copy += ' ';
        wlist_copy += words;
      }

      return true;
    }

    /*!
     * \brief Converts bytes (secret key) to seed words.
     * \param  src           Secret key
     * \param  words         Space delimited concatenated words get written here.
     * \param  language_name Seed language name
     * \return               true if successful false if not. Unsuccessful if wrong key size.
     */
    bool bytes_to_words(const crypto::secret_key& src, std::string& words,
      const std::string &language_name)
    {

      if (sizeof(src.data) % 4 != 0 || sizeof(src.data) == 0) return false;

      std::vector<std::string> word_list;
      Language::Base *language;
      if (language_name == "English")
      {
        language = Language::Singleton<Language::English>::instance();
      }
      else if (language_name == "Spanish")
      {
        language = Language::Singleton<Language::Spanish>::instance();
      }
      else if (language_name == "Portuguese")
      {
        language = Language::Singleton<Language::Portuguese>::instance();
      }
      else if (language_name == "Japanese")
      {
        language = Language::Singleton<Language::Japanese>::instance();
      }
      else
      {
        return false;
      }
      word_list = language->get_word_list();
      // To store the words for random access to add the checksum word later.
      std::vector<std::string> words_store;

      uint32_t word_list_length = word_list.size();
      // 8 bytes -> 3 words.  8 digits base 16 -> 3 digits base 1626
      for (unsigned int i=0; i < sizeof(src.data)/4; i++, words += ' ')
      {
        uint32_t w1, w2, w3;
        
        uint32_t val;

        memcpy(&val, (src.data) + (i * 4), 4);

        w1 = val % word_list_length;
        w2 = ((val / word_list_length) + w1) % word_list_length;
        w3 = (((val / word_list_length) / word_list_length) + w2) % word_list_length;

        words += word_list[w1];
        words += ' ';
        words += word_list[w2];
        words += ' ';
        words += word_list[w3];

        words_store.push_back(word_list[w1]);
        words_store.push_back(word_list[w2]);
        words_store.push_back(word_list[w3]);
      }

      words.pop_back();
      words += (' ' + words_store[create_checksum_index(words_store)]);
      return false;
    }

    /*!
     * \brief Gets a list of seed languages that are supported.
     * \param languages The vector is set to the list of languages.
     */
    void get_language_list(std::vector<std::string> &languages)
    {
      std::vector<Language::Base*> language_instances({
        Language::Singleton<Language::English>::instance(),
        Language::Singleton<Language::Spanish>::instance(),
        Language::Singleton<Language::Portuguese>::instance(),
        Language::Singleton<Language::Japanese>::instance(),
        Language::Singleton<Language::OldEnglish>::instance()
      });
      for (std::vector<Language::Base*>::iterator it = language_instances.begin();
        it != language_instances.end(); it++)
      {
        languages.push_back((*it)->get_language_name());
      }
    }

    /*!
     * \brief Tells if the seed passed is an old style seed or not.
     * \param  seed The seed to check (a space delimited concatenated word list)
     * \return      true if the seed passed is a old style seed false if not.
     */
    bool get_is_old_style_seed(const std::string &seed)
    {
      std::vector<std::string> word_list;
      boost::split(word_list, seed, boost::is_any_of(" "));
      return word_list.size() != (seed_length + 1);
    }

  }

}
