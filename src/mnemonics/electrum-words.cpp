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
#include "wipeable_string.h"
#include "misc_language.h"
#include "crypto/crypto.h"  // for declaration of crypto::secret_key
#include <fstream>
#include "common/int-util.h"
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

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "mnemonic"

namespace crypto
{
  namespace ElectrumWords
  {
    std::vector<const Language::Base*> get_language_list();
  }
}

namespace
{
  uint32_t create_checksum_index(const std::vector<epee::wipeable_string> &word_list,
    uint32_t unique_prefix_length);
  bool checksum_test(std::vector<epee::wipeable_string> seed, uint32_t unique_prefix_length);

  /*!
   * \brief Finds the word list that contains the seed words and puts the indices
   *        where matches occured in matched_indices.
   * \param  seed            List of words to match.
   * \param  has_checksum    The seed has a checksum word (maybe not checked).
   * \param  matched_indices The indices where the seed words were found are added to this.
   * \param  language        Language instance pointer to write to after it is found.
   * \return                 true if all the words were present in some language false if not.
   */
  bool find_seed_language(const std::vector<epee::wipeable_string> &seed,
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

    std::vector<epee::wipeable_string>::const_iterator it2;
    matched_indices.reserve(seed.size());

    // Iterate through all the languages and find a match
    for (std::vector<Language::Base*>::iterator it1 = language_instances.begin();
      it1 != language_instances.end(); it1++)
    {
      const std::unordered_map<epee::wipeable_string, uint32_t> &word_map = (*it1)->get_word_map();
      const std::unordered_map<epee::wipeable_string, uint32_t> &trimmed_word_map = (*it1)->get_trimmed_word_map();
      // To iterate through seed words
      bool full_match = true;

      epee::wipeable_string trimmed_word;
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
        MINFO("Full match for language " << (*language)->get_english_language_name());
        return true;
      }
      // Some didn't match. Clear the index array.
      memwipe(matched_indices.data(), matched_indices.size() * sizeof(matched_indices[0]));
      matched_indices.clear();
    }

    // if we get there, we've not found a good match, but we might have a fallback,
    // if we detected a match which did not fit the checksum, which might be a badly
    // typed/transcribed seed in the right language
    if (fallback)
    {
      *language = fallback;
      MINFO("Fallback match for language " << (*language)->get_english_language_name());
      return true;
    }

    MINFO("No match found");
    memwipe(matched_indices.data(), matched_indices.size() * sizeof(matched_indices[0]));
    return false;
  }

  /*!
   * \brief Creates a checksum index in the word list array on the list of words.
   * \param  word_list            Vector of words
   * \param unique_prefix_length  the prefix length of each word to use for checksum
   * \return                      Checksum index
   */
  uint32_t create_checksum_index(const std::vector<epee::wipeable_string> &word_list,
    uint32_t unique_prefix_length)
  {
    epee::wipeable_string trimmed_words = "";

    for (std::vector<epee::wipeable_string>::const_iterator it = word_list.begin(); it != word_list.end(); it++)
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
  bool checksum_test(std::vector<epee::wipeable_string> seed, uint32_t unique_prefix_length)
  {
    if (seed.empty())
      return false;
    // The last word is the checksum.
    epee::wipeable_string last_word = seed.back();
    seed.pop_back();

    epee::wipeable_string checksum = seed[create_checksum_index(seed, unique_prefix_length)];

    epee::wipeable_string trimmed_checksum = checksum.length() > unique_prefix_length ? Language::utf8prefix(checksum, unique_prefix_length) :
      checksum;
    epee::wipeable_string trimmed_last_word = last_word.length() > unique_prefix_length ? Language::utf8prefix(last_word, unique_prefix_length) :
      last_word;
    bool ret = trimmed_checksum == trimmed_last_word;
    MINFO("Checksum is " << (ret ? "valid" : "invalid"));
    return ret;
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
    bool words_to_bytes(const epee::wipeable_string &words, epee::wipeable_string& dst, size_t len, bool duplicate,
      std::string &language_name)
    {
      std::vector<epee::wipeable_string> seed;

      words.split(seed);

      if (len % 4)
      {
        MERROR("Invalid seed: not a multiple of 4");
        return false;
      }

      bool has_checksum = true;
      if (len)
      {
        // error on non-compliant word list
        const size_t expected = len * 8 * 3 / 32;
        if (seed.size() != expected/2 && seed.size() != expected &&
          seed.size() != expected + 1)
        {
          MERROR("Invalid seed: unexpected number of words");
          return false;
        }

        // If it is seed with a checksum.
        has_checksum = seed.size() == (expected + 1);
      }

      std::vector<uint32_t> matched_indices;
      auto wiper = epee::misc_utils::create_scope_leave_handler([&](){memwipe(matched_indices.data(), matched_indices.size() * sizeof(matched_indices[0]));});
      Language::Base *language;
      if (!find_seed_language(seed, has_checksum, matched_indices, &language))
      {
        MERROR("Invalid seed: language not found");
        return false;
      }
      language_name = language->get_language_name();
      uint32_t word_list_length = language->get_word_list().size();

      if (has_checksum)
      {
        if (!checksum_test(seed, language->get_unique_prefix_length()))
        {
          // Checksum fail
          MERROR("Invalid seed: invalid checksum");
          return false;
        }
        seed.pop_back();
      }

      for (unsigned int i=0; i < seed.size() / 3; i++)
      {
        uint32_t w[4];
        w[1] = matched_indices[i*3];
        w[2] = matched_indices[i*3 + 1];
        w[3] = matched_indices[i*3 + 2];

        w[0]= w[1] + word_list_length * (((word_list_length - w[1]) + w[2]) % word_list_length) +
          word_list_length * word_list_length * (((word_list_length - w[2]) + w[3]) % word_list_length);

        if (!(w[0]% word_list_length == w[1]))
        {
          memwipe(w, sizeof(w));
          MERROR("Invalid seed: mumble mumble");
          return false;
        }

        dst.append((const char*)&w[0], 4);  // copy 4 bytes to position
        memwipe(w, sizeof(w));
      }

      if (len > 0 && duplicate)
      {
        const size_t expected = len * 3 / 32;
        if (seed.size() == expected/2)
        {
          dst += ' ';                    // if electrum 12-word seed, duplicate
          dst += dst;                    // if electrum 12-word seed, duplicate
          dst.pop_back();                // trailing space
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
    bool words_to_bytes(const epee::wipeable_string &words, crypto::secret_key& dst,
      std::string &language_name)
    {
      epee::wipeable_string s;
      if (!words_to_bytes(words, s, sizeof(dst), true, language_name))
      {
        MERROR("Invalid seed: failed to convert words to bytes");
        return false;
      }
      if (s.size() != sizeof(dst))
      {
        MERROR("Invalid seed: wrong output size");
        return false;
      }
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
    bool bytes_to_words(const char *src, size_t len, epee::wipeable_string& words,
      const std::string &language_name)
    {

      if (len % 4 != 0 || len == 0) return false;

      const Language::Base *language = NULL;
      const std::vector<const Language::Base*> language_list = crypto::ElectrumWords::get_language_list();
      for (const Language::Base *l: language_list)
      {
        if (language_name == l->get_language_name() || language_name == l->get_english_language_name())
          language = l;
      }
      if (!language)
      {
        return false;
      }
      const std::vector<std::string> &word_list = language->get_word_list();
      // To store the words for random access to add the checksum word later.
      std::vector<epee::wipeable_string> words_store;

      uint32_t word_list_length = word_list.size();
      // 4 bytes -> 3 words.  8 digits base 16 -> 3 digits base 1626
      for (unsigned int i=0; i < len/4; i++, words.push_back(' '))
      {
        uint32_t w[4];

        w[0] = SWAP32LE(*(const uint32_t*)(src + (i * 4)));

        w[1] = w[0] % word_list_length;
        w[2] = ((w[0] / word_list_length) + w[1]) % word_list_length;
        w[3] = (((w[0] / word_list_length) / word_list_length) + w[2]) % word_list_length;

        words += word_list[w[1]];
        words += ' ';
        words += word_list[w[2]];
        words += ' ';
        words += word_list[w[3]];

        words_store.push_back(word_list[w[1]]);
        words_store.push_back(word_list[w[2]]);
        words_store.push_back(word_list[w[3]]);

        memwipe(w, sizeof(w));
      }

      words += words_store[create_checksum_index(words_store, language->get_unique_prefix_length())];
      return true;
    }

    bool bytes_to_words(const crypto::secret_key& src, epee::wipeable_string& words,
      const std::string &language_name)
    {
      return bytes_to_words(src.data, sizeof(src), words, language_name);
    }

    std::vector<const Language::Base*> get_language_list()
    {
      static const std::vector<const Language::Base*> language_instances({
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
      return language_instances;
    }

    /*!
     * \brief Gets a list of seed languages that are supported.
     * \param languages The vector is set to the list of languages.
     */
    void get_language_list(std::vector<std::string> &languages, bool english)
    {
      const std::vector<const Language::Base*> language_instances = get_language_list();
      for (std::vector<const Language::Base*>::const_iterator it = language_instances.begin();
        it != language_instances.end(); it++)
      {
        languages.push_back(english ? (*it)->get_english_language_name() : (*it)->get_language_name());
      }
    }

    /*!
     * \brief Tells if the seed passed is an old style seed or not.
     * \param  seed The seed to check (a space delimited concatenated word list)
     * \return      true if the seed passed is a old style seed false if not.
     */
    bool get_is_old_style_seed(const epee::wipeable_string &seed)
    {
      std::vector<epee::wipeable_string> word_list;
      seed.split(word_list);
      return word_list.size() != (seed_length + 1);
    }

    std::string get_english_name_for(const std::string &name)
    {
      const std::vector<const Language::Base*> language_instances = get_language_list();
      for (std::vector<const Language::Base*>::const_iterator it = language_instances.begin();
        it != language_instances.end(); it++)
      {
        if ((*it)->get_language_name() == name)
          return (*it)->get_english_language_name();
      }
      return "<language not found>";
    }

  }

}
