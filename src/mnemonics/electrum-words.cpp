// Copyright (c) 2014-2018, The Monero Project
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

#include "chinese_simplified.h"
#include "english.h"
#include "dutch.h"
#include "french.h"
#include "italian.h"
#include "german.h"
#include "spanish.h"
#include "portuguese.h"
#include "japanese.h"
#include "russian.h"
#include "esperanto.h"
#include "lojban.h"
#include "english_old.h"
#include "language_base.h"
#include "singleton.h"

namespace
{
  uint32_t create_checksum_index(const std::vector<std::string> &word_list,
    uint32_t unique_prefix_length);
  bool checksum_test(std::vector<std::string> seed, uint32_t unique_prefix_length);

  /*!
   * \brief Finds the word list that contains the seed words and puts the indices
   *        where matches occured in matched_indices.
   * \param  seed            List of words to match.
   * \param  has_checksum    The seed has a checksum word (maybe not checked).
   * \param  matched_indices The indices where the seed words were found are added to this.
   * \param  language        Language instance pointer to write to after it is found.
   * \return                 true if all the words were present in some language false if not.
   */
  bool find_seed_language(const std::vector<std::string> &seed,
    bool has_checksum, std::vector<uint32_t> &matched_indices, Language::Base **language)
  {
    // If there's a new language added, add an instance of it here.
    std::vector<Language::Base*> language_instances({
      Language::Singleton<Language::Chinese_Simplified>::instance(),
      Language::Singleton<Language::English>::instance(),
      Language::Singleton<Language::Dutch>::instance(),
      Language::Singleton<Language::French>::instance(),
      Language::Singleton<Language::Spanish>::instance(),
      Language::Singleton<Language::German>::instance(),
      Language::Singleton<Language::Italian>::instance(),
      Language::Singleton<Language::Portuguese>::instance(),
      Language::Singleton<Language::Japanese>::instance(),
      Language::Singleton<Language::Russian>::instance(),
      Language::Singleton<Language::Esperanto>::instance(),
      Language::Singleton<Language::Lojban>::instance(),
      Language::Singleton<Language::EnglishOld>::instance()
    });
    Language::Base *fallback = NULL;

    // Iterate through all the languages and find a match
    for (std::vector<Language::Base*>::iterator it1 = language_instances.begin();
      it1 != language_instances.end(); it1++)
    {
      const std::unordered_map<std::string, uint32_t> &word_map = (*it1)->get_word_map();
      const std::unordered_map<std::string, uint32_t> &trimmed_word_map = (*it1)->get_trimmed_word_map();
      // To iterate through seed words
      std::vector<std::string>::const_iterator it2;
      bool full_match = true;

      std::string trimmed_word;
      // Iterate through all the words and see if they're all present
      for (it2 = seed.begin(); it2 != seed.end(); it2++)
      {
        if (has_checksum)
        {
          trimmed_word = Language::utf8prefix(*it2, (*it1)->get_unique_prefix_length());
          // Use the trimmed words and map
          if (trimmed_word_map.count(trimmed_word) == 0)
          {
            full_match = false;
            break;
          }
          matched_indices.push_back(trimmed_word_map.at(trimmed_word));
        }
        else
        {
          if (word_map.count(*it2) == 0)
          {
            full_match = false;
            break;
          }
          matched_indices.push_back(word_map.at(*it2));
        }
      }
      if (full_match)
      {
        // if we were using prefix only, and we have a checksum, check it now
        // to avoid false positives due to prefix set being too common
        if (has_checksum)
          if (!checksum_test(seed, (*it1)->get_unique_prefix_length()))
          {
            fallback = *it1;
            full_match = false;
          }
      }
      if (full_match)
      {
        *language = *it1;
        return true;
      }
      // Some didn't match. Clear the index array.
      matched_indices.clear();
    }

    // if we get there, we've not found a good match, but we might have a fallback,
    // if we detected a match which did not fit the checksum, which might be a badly
    // typed/transcribed seed in the right language
    if (fallback)
    {
      *language = fallback;
      return true;
    }

    return false;
  }

  /*!
   * \brief Creates a checksum index in the word list array on the list of words.
   * \param  word_list            Vector of words
   * \param unique_prefix_length  the prefix length of each word to use for checksum
   * \return                      Checksum index
   */
  uint32_t create_checksum_index(const std::vector<std::string> &word_list,
    uint32_t unique_prefix_length)
  {
    std::string trimmed_words = "";

    for (std::vector<std::string>::const_iterator it = word_list.begin(); it != word_list.end(); it++)
    {
      if (it->length() > unique_prefix_length)
      {
        trimmed_words += Language::utf8prefix(*it, unique_prefix_length);
      }
      else
      {
        trimmed_words += *it;
      }
    }
    boost::crc_32_type result;
    result.process_bytes(trimmed_words.data(), trimmed_words.length());
    return result.checksum() % crypto::ElectrumWords::seed_length;
  }

  /*!
   * \brief Does the checksum test on the seed passed.
   * \param seed                  Vector of seed words
   * \param unique_prefix_length  the prefix length of each word to use for checksum
   * \return                      True if the test passed false if not.
   */
  bool checksum_test(std::vector<std::string> seed, uint32_t unique_prefix_length)
  {
    if (seed.empty())
      return false;
    // The last word is the checksum.
    std::string last_word = seed.back();
    seed.pop_back();

    std::string checksum = seed[create_checksum_index(seed, unique_prefix_length)];

    std::string trimmed_checksum = checksum.length() > unique_prefix_length ? Language::utf8prefix(checksum, unique_prefix_length) :
      checksum;
    std::string trimmed_last_word = last_word.length() > unique_prefix_length ? Language::utf8prefix(last_word, unique_prefix_length) :
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
     * \param  dst             To put the secret data restored from the words.
     * \param  len             The number of bytes to expect, 0 if unknown
     * \param  duplicate       If true and len is not zero, we accept half the data, and duplicate it
     * \param  language_name   Language of the seed as found gets written here.
     * \return                 false if not a multiple of 3 words, or if word is not in the words list
     */
    bool words_to_bytes(std::string words, std::string& dst, size_t len, bool duplicate,
      std::string &language_name)
    {
      std::vector<std::string> seed;

      boost::algorithm::trim(words);
      boost::split(seed, words, boost::is_any_of(" "), boost::token_compress_on);

      if (len % 4)
        return false;

      bool has_checksum = true;
      if (len)
      {
        // error on non-compliant word list
        const size_t expected = len * 8 * 3 / 32;
        if (seed.size() != expected/2 && seed.size() != expected &&
          seed.size() != expected + 1)
        {
          return false;
        }

        // If it is seed with a checksum.
        has_checksum = seed.size() == (expected + 1);
      }

      std::vector<uint32_t> matched_indices;
      Language::Base *language;
      if (!find_seed_language(seed, has_checksum, matched_indices, &language))
      {
        return false;
      }
      language_name = language->get_language_name();
      uint32_t word_list_length = language->get_word_list().size();

      if (has_checksum)
      {
        if (!checksum_test(seed, language->get_unique_prefix_length()))
        {
          // Checksum fail
          return false;
        }
        seed.pop_back();
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

        dst.append((const char*)&val, 4);  // copy 4 bytes to position
      }

      if (len > 0 && duplicate)
      {
        const size_t expected = len * 3 / 32;
        std::string wlist_copy = words;
        if (seed.size() == expected/2)
        {
          dst.append(dst);                    // if electrum 12-word seed, duplicate
          wlist_copy += ' ';
          wlist_copy += words;
        }
      }

      return true;
    }

    /*!
     * \brief Converts seed words to bytes (secret key).
     * \param  words           String containing the words separated by spaces.
     * \param  dst             To put the secret key restored from the words.
     * \param  language_name   Language of the seed as found gets written here.
     * \return                 false if not a multiple of 3 words, or if word is not in the words list
     */
    bool words_to_bytes(std::string words, crypto::secret_key& dst,
      std::string &language_name)
    {
      std::string s;
      if (!words_to_bytes(words, s, sizeof(dst), true, language_name))
        return false;
      if (s.size() != sizeof(dst))
        return false;
      dst = *(const crypto::secret_key*)s.data();
      return true;
    }

    /*!
     * \brief Converts bytes (secret key) to seed words.
     * \param  src           Secret key
     * \param  words         Space delimited concatenated words get written here.
     * \param  language_name Seed language name
     * \return               true if successful false if not. Unsuccessful if wrong key size.
     */
    bool bytes_to_words(const char *src, size_t len, std::string& words,
      const std::string &language_name)
    {

      if (len % 4 != 0 || len == 0) return false;

      Language::Base *language;
      if (language_name == "English")
      {
        language = Language::Singleton<Language::English>::instance();
      }
      else if (language_name == "Nederlands")
      {
        language = Language::Singleton<Language::Dutch>::instance();
      }
      else if (language_name == "Français")
      {
        language = Language::Singleton<Language::French>::instance();
      }
      else if (language_name == "Español")
      {
        language = Language::Singleton<Language::Spanish>::instance();
      }
      else if (language_name == "Português")
      {
        language = Language::Singleton<Language::Portuguese>::instance();
      }
      else if (language_name == "日本語")
      {
        language = Language::Singleton<Language::Japanese>::instance();
      }
      else if (language_name == "Italiano")
      {
        language = Language::Singleton<Language::Italian>::instance();
      }
      else if (language_name == "Deutsch")
      {
        language = Language::Singleton<Language::German>::instance();
      }
      else if (language_name == "русский язык")
      {
        language = Language::Singleton<Language::Russian>::instance();
      }
      else if (language_name == "简体中文 (中国)")
      {
        language = Language::Singleton<Language::Chinese_Simplified>::instance();
      }
      else if (language_name == "Esperanto")
      {
        language = Language::Singleton<Language::Esperanto>::instance();
      }
      else if (language_name == "Lojban")
      {
        language = Language::Singleton<Language::Lojban>::instance();
      }
      else
      {
        return false;
      }
      const std::vector<std::string> &word_list = language->get_word_list();
      // To store the words for random access to add the checksum word later.
      std::vector<std::string> words_store;

      uint32_t word_list_length = word_list.size();
      // 4 bytes -> 3 words.  8 digits base 16 -> 3 digits base 1626
      for (unsigned int i=0; i < len/4; i++, words += ' ')
      {
        uint32_t w1, w2, w3;
        
        uint32_t val;

        memcpy(&val, src + (i * 4), 4);

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
      words += (' ' + words_store[create_checksum_index(words_store, language->get_unique_prefix_length())]);
      return true;
    }

    bool bytes_to_words(const crypto::secret_key& src, std::string& words,
      const std::string &language_name)
    {
      return bytes_to_words(src.data, sizeof(src), words, language_name);
    }

    /*!
     * \brief Gets a list of seed languages that are supported.
     * \param languages The vector is set to the list of languages.
     */
    void get_language_list(std::vector<std::string> &languages)
    {
      std::vector<Language::Base*> language_instances({
        Language::Singleton<Language::German>::instance(),
        Language::Singleton<Language::English>::instance(),
        Language::Singleton<Language::Spanish>::instance(),
        Language::Singleton<Language::French>::instance(),
        Language::Singleton<Language::Italian>::instance(),
        Language::Singleton<Language::Dutch>::instance(),
        Language::Singleton<Language::Portuguese>::instance(),
        Language::Singleton<Language::Russian>::instance(),
        Language::Singleton<Language::Japanese>::instance(),
        Language::Singleton<Language::Chinese_Simplified>::instance(),
        Language::Singleton<Language::Esperanto>::instance(),
        Language::Singleton<Language::Lojban>::instance()
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
    bool get_is_old_style_seed(std::string seed)
    {
      std::vector<std::string> word_list;
      boost::algorithm::trim(seed);
      boost::split(word_list, seed, boost::is_any_of(" "), boost::token_compress_on);
      return word_list.size() != (seed_length + 1);
    }

  }

}
