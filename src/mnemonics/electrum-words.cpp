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
#include <boost/algorithm/string.hpp>
#include "crypto/crypto.h"  // for declaration of crypto::secret_key
#include <fstream>
#include "mnemonics/electrum-words.h"
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/crc.hpp>
#include <boost/algorithm/string/join.hpp>

namespace
{
  int num_words = 0;
  const int seed_length = 24;

  std::map<std::string,uint32_t> words_map;
  std::vector<std::string> words_array;

  bool is_old_style_word_list = false;

  const std::string WORD_LISTS_DIRECTORY = "wordlists";
  const std::string LANGUAGES_DIRECTORY = "languages";
  const std::string OLD_WORD_FILE = "old-word-list";

  const int unique_prefix_length = 4;
  /*!
   * \brief Tells if the module hasn't been initialized with a word list file.
   * \return true if the module hasn't been initialized with a word list file false otherwise.
   */
  bool is_uninitialized()
  {
    return num_words == 0 ? true : false;
  }

  /*!
   * \brief Create word list map and array data structres for use during inter-conversion between
   * words and secret key.
   * \param word_file     Path to the word list file from pwd.
   * \param has_checksum  True if checksum was supplied false if not.
   */
  void create_data_structures(const std::string &word_file, bool has_checksum)
  {
    words_array.clear();
    words_map.clear();
    num_words = 0;
    std::ifstream input_stream;
    input_stream.open(word_file.c_str(), std::ifstream::in);

    if (!input_stream)
      throw std::runtime_error("Word list file couldn't be opened.");

    std::string word;
    while (input_stream >> word)
    {
      if (word.length() == 0 || word[0] == '#')
      {
        // Skip empty and comment lines
        continue;
      }
      words_array.push_back(word);
      if (has_checksum)
      {
        // Only if checksum was passed should we stick to just 4 char checks to be lenient about typos.
        words_map[word.substr(0, unique_prefix_length)] = num_words;
      }
      else
      {
        words_map[word] = num_words;
      }
      num_words++;
    }
    input_stream.close();
  }

  /*!
   * \brief Tells if all the words passed in wlist was present in current word list file.
   * \param  wlist        List of words to match.
   * \param  has_checksum If word list passed checksum test, we need to only do a 4 char check.
   * \return              true if all the words were present false if not.
   */
  bool word_list_file_match(const std::vector<std::string> &wlist, bool has_checksum)
  {
    for (std::vector<std::string>::const_iterator it = wlist.begin(); it != wlist.end(); it++)
    {
      if (has_checksum)
      {
        if (words_map.count(it->substr(0, unique_prefix_length)) == 0)
        {
          return false;
        }
      }
      else
      {
        if (words_map.count(*it) == 0)
        {
          return false;
        }
      }
    }
    return true;
  }

  /*!
   * \brief Creates a checksum index in the word list array on the list of words.
   * \param  word_list Vector of words
   * \return           Checksum index
   */
  uint32_t create_checksum_index(const std::vector<std::string> &word_list)
  {
    std::string four_char_words = "";

    for (std::vector<std::string>::const_iterator it = word_list.begin(); it != word_list.end(); it++)
    {
      four_char_words += it->substr(0, unique_prefix_length);
    }
    boost::crc_32_type result;
    result.process_bytes(four_char_words.data(), four_char_words.length());
    return result.checksum() % seed_length;
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
     * \brief Called to initialize it to work with a word list file.
     * \param language          Language of the word list file.
     * \param has_checksum      True if the checksum was passed false if not.
     * \param old_word_list     true it is to use the old style word list file false if not.
     */
    void init(const std::string &language, bool has_checksum, bool old_word_list)
    {
      if (old_word_list)
      {
        // Use the old word list file if told to.
        create_data_structures(WORD_LISTS_DIRECTORY + '/' + OLD_WORD_FILE, has_checksum);
        is_old_style_word_list = true;
      }
      else
      {
        create_data_structures(WORD_LISTS_DIRECTORY + '/' + LANGUAGES_DIRECTORY + '/' + language, has_checksum);
        is_old_style_word_list = false;
      }
      if (num_words == 0)
      {
        throw std::runtime_error(std::string("Word list file is empty: ") +
          (old_word_list ? OLD_WORD_FILE : (LANGUAGES_DIRECTORY + '/' + language)));
      }
    }

    /*!
     * \brief Converts seed words to bytes (secret key).
     * \param  words String containing the words separated by spaces.
     * \param  dst   To put the secret key restored from the words.
     * \return       false if not a multiple of 3 words, or if word is not in the words list
     */
    bool words_to_bytes(const std::string& words, crypto::secret_key& dst)
    {
      std::vector<std::string> wlist;

      boost::split(wlist, words, boost::is_any_of(" "));

      // If it is seed with a checksum.
      bool has_checksum = (wlist.size() == seed_length + 1);

      if (has_checksum)
      {
        // The last word is the checksum.
        std::string last_word = wlist.back();
        wlist.pop_back();

        std::string checksum = wlist[create_checksum_index(wlist)];

        if (checksum.substr(0, unique_prefix_length) != last_word.substr(0, unique_prefix_length))
        {
          // Checksum fail
          return false;
        }
      }
      // Try to find a word list file that contains all the words in the word list.
      std::vector<std::string> languages;
      get_language_list(languages);

      std::vector<std::string>::iterator it;
      for (it = languages.begin(); it != languages.end(); it++)
      {
        init(*it, has_checksum);
        if (word_list_file_match(wlist, has_checksum))
        {
          break;
        }
      }
      // If no such file was found, see if the old style word list file has them all.
      if (it == languages.end())
      {
        init("", has_checksum, true);
        if (!word_list_file_match(wlist, has_checksum))
        {
          return false;
        }
      }
      int n = num_words;

      // error on non-compliant word list
      if (wlist.size() != 12 && wlist.size() != 24) return false;

      for (unsigned int i=0; i < wlist.size() / 3; i++)
      {
        uint32_t val;
        uint32_t w1, w2, w3;

        if (has_checksum)
        {
          w1 = words_map.at(wlist[i*3].substr(0, unique_prefix_length));
          w2 = words_map.at(wlist[i*3 + 1].substr(0, unique_prefix_length));
          w3 = words_map.at(wlist[i*3 + 2].substr(0, unique_prefix_length));
        }
        else
        {
          w1 = words_map.at(wlist[i*3]);
          w2 = words_map.at(wlist[i*3 + 1]);
          w3 = words_map.at(wlist[i*3 + 2]);
        }

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

    /*!
     * \brief Converts bytes (secret key) to seed words.
     * \param  src   Secret key
     * \param  words Space delimited concatenated words get written here.
     * \return       true if successful false if not. Unsuccessful if wrong key size.
     */
    bool bytes_to_words(const crypto::secret_key& src, std::string& words)
    {
      if (is_uninitialized())
      {
        init("english", true);
      }

      // To store the words for random access to add the checksum word later.
      std::vector<std::string> words_store;
      int n = num_words;

      if (sizeof(src.data) % 4 != 0 || sizeof(src.data) == 0) return false;

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

        words_store.push_back(words_array[w1]);
        words_store.push_back(words_array[w2]);
        words_store.push_back(words_array[w3]);
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
      languages.clear();
      boost::filesystem::path languages_directory("wordlists/languages");
      if (!boost::filesystem::exists(languages_directory) ||
        !boost::filesystem::is_directory(languages_directory))
      {
        throw std::runtime_error("Word list languages directory is missing.");
      }
      boost::filesystem::directory_iterator end;
      for (boost::filesystem::directory_iterator it(languages_directory); it != end; it++)
      {
        languages.push_back(it->path().filename().string());
      }
    }

    /*!
     * \brief Tells if the module is currenly using an old style word list.
     * \return true if it is currenly using an old style word list false if not.
     */
    bool get_is_old_style_word_list()
    {
      if (is_uninitialized())
      {
        throw std::runtime_error("ElectrumWords hasn't been initialized with a word list yet.");
      }
      return is_old_style_word_list;
    }

    /*!
     * \brief Tells if the seed passed is an old style seed or not.
     * \param  seed The seed to check (a space delimited concatenated word list)
     * \return      true if the seed passed is a old style seed false if not.
     */
    bool get_is_old_style_seed(const std::string &seed)
    {
      std::vector<std::string> wlist;
      boost::split(wlist, seed, boost::is_any_of(" "));
      return wlist.size() != (seed_length + 1);
    }

  }

}
