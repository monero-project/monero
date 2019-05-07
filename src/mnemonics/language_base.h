// Copyright (c) 2014-2019, The Monero Project
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
 * \file language_base.h
 * 
 * \brief Language Base class for Polymorphism.
 */

#ifndef LANGUAGE_BASE_H
#define LANGUAGE_BASE_H

#include <vector>
#include <unordered_map>
#include <string>
#include <boost/algorithm/string.hpp>
#include "misc_log_ex.h"
#include "fnv1.h"

/*!
 * \namespace Language
 * \brief Mnemonic language related namespace.
 */
namespace Language
{
  /*!
   * \brief Returns a string made of (at most) the first count characters in s.
   *        Assumes well formedness. No check is made for this.
   * \param  s               The string from which to return the first count characters.
   * \param  count           How many characters to return.
   * \return                 A string consisting of the first count characters in s.
   */
  template<typename T>
  inline T utf8prefix(const T &s, size_t count)
  {
    T prefix = "";
    size_t avail = s.size();
    const char *ptr = s.data();
    while (count-- && avail--)
    {
      prefix += *ptr++;
      while (avail && ((*ptr) & 0xc0) == 0x80)
      {
        prefix += *ptr++;
        --avail;
      }
    }
    return prefix;
  }

  template<typename T>
  inline T utf8canonical(const T &s)
  {
    T sc = "";
    size_t avail = s.size();
    const char *ptr = s.data();
    wint_t cp = 0;
    int bytes = 1;
    char wbuf[8], *wptr;
    while (avail--)
    {
      if ((*ptr & 0x80) == 0)
      {
        cp = *ptr++;
        bytes = 1;
      }
      else if ((*ptr & 0xe0) == 0xc0)
      {
        if (avail < 1)
          throw std::runtime_error("Invalid UTF-8");
        cp = (*ptr++ & 0x1f) << 6;
        cp |= *ptr++ & 0x3f;
        --avail;
        bytes = 2;
      }
      else if ((*ptr & 0xf0) == 0xe0)
      {
        if (avail < 2)
          throw std::runtime_error("Invalid UTF-8");
        cp = (*ptr++ & 0xf) << 12;
        cp |= (*ptr++ & 0x3f) << 6;
        cp |= *ptr++ & 0x3f;
        avail -= 2;
        bytes = 3;
      }
      else if ((*ptr & 0xf8) == 0xf0)
      {
        if (avail < 3)
          throw std::runtime_error("Invalid UTF-8");
        cp = (*ptr++ & 0x7) << 18;
        cp |= (*ptr++ & 0x3f) << 12;
        cp |= (*ptr++ & 0x3f) << 6;
        cp |= *ptr++ & 0x3f;
        avail -= 3;
        bytes = 4;
      }
      else
        throw std::runtime_error("Invalid UTF-8");

      cp = std::towlower(cp);
      wptr = wbuf;
      switch (bytes)
      {
        case 1: *wptr++ = cp; break;
        case 2: *wptr++ = 0xc0 | (cp >> 6); *wptr++ = 0x80 | (cp & 0x3f); break;
        case 3: *wptr++ = 0xe0 | (cp >> 12); *wptr++ = 0x80 | ((cp >> 6) & 0x3f); *wptr++ = 0x80 | (cp & 0x3f); break;
        case 4: *wptr++ = 0xf0 | (cp >> 18); *wptr++ = 0x80 | ((cp >> 12) & 0x3f); *wptr++ = 0x80 | ((cp >> 6) & 0x3f); *wptr++ = 0x80 | (cp & 0x3f); break;
        default: throw std::runtime_error("Invalid UTF-8");
      }
      *wptr = 0;
      sc += T(wbuf, bytes);
      cp = 0;
      bytes = 1;
    }
    return sc;
  }

  struct WordHash
  {
    std::size_t operator()(const epee::wipeable_string &s) const
    {
      const epee::wipeable_string sc = utf8canonical(s);
      return epee::fnv::FNV1a(sc.data(), sc.size());
    }
  };

  struct WordEqual
  {
    bool operator()(const epee::wipeable_string &s0, const epee::wipeable_string &s1) const
    {
      const epee::wipeable_string s0c = utf8canonical(s0);
      const epee::wipeable_string s1c = utf8canonical(s1);
      return s0c == s1c;
    }
  };

  /*!
   * \class Base
   * \brief A base language class which all languages have to inherit from for
   * Polymorphism.
   */
  class Base
  {
  protected:
    enum {
      ALLOW_SHORT_WORDS = 1<<0,
      ALLOW_DUPLICATE_PREFIXES = 1<<1,
    };
    enum {
      NWORDS = 1626
    };
    std::vector<std::string> word_list; /*!< A pointer to the array of words */
    std::unordered_map<epee::wipeable_string, uint32_t, WordHash, WordEqual> word_map; /*!< hash table to find word's index */
    std::unordered_map<epee::wipeable_string, uint32_t, WordHash, WordEqual> trimmed_word_map; /*!< hash table to find word's trimmed index */
    std::string language_name; /*!< Name of language */
    std::string english_language_name; /*!< Name of language */
    uint32_t unique_prefix_length; /*!< Number of unique starting characters to trim the wordlist to when matching */
    /*!
     * \brief Populates the word maps after the list is ready.
     */
    void populate_maps(uint32_t flags = 0)
    {
      int ii;
      std::vector<std::string>::const_iterator it;
      if (word_list.size () != NWORDS)
        throw std::runtime_error("Wrong word list length for " + language_name);
      for (it = word_list.begin(), ii = 0; it != word_list.end(); it++, ii++)
      {
        word_map[*it] = ii;
        if ((*it).size() < unique_prefix_length)
        {
          if (flags & ALLOW_SHORT_WORDS)
            MWARNING(language_name << " word '" << *it << "' is shorter than its prefix length, " << unique_prefix_length);
          else
            throw std::runtime_error("Too short word in " + language_name + " word list: " + *it);
        }
        epee::wipeable_string trimmed;
        if (it->length() > unique_prefix_length)
        {
          trimmed = utf8prefix(*it, unique_prefix_length);
        }
        else
        {
          trimmed = *it;
        }
        if (trimmed_word_map.find(trimmed) != trimmed_word_map.end())
        {
          if (flags & ALLOW_DUPLICATE_PREFIXES)
            MWARNING("Duplicate prefix in " << language_name << " word list: " << std::string(trimmed.data(), trimmed.size()));
          else
            throw std::runtime_error("Duplicate prefix in " + language_name + " word list: " + std::string(trimmed.data(), trimmed.size()));
        }
        trimmed_word_map[trimmed] = ii;
      }
    }
  public:
    Base(const char *language_name, const char *english_language_name, const std::vector<std::string> &words, uint32_t prefix_length):
      word_list(words),
      unique_prefix_length(prefix_length),
      language_name(language_name),
      english_language_name(english_language_name)
    {
    }
    virtual ~Base()
    {
    }
    void set_words(const char * const words[])
    {
      word_list.resize(NWORDS);
      for (size_t i = 0; i < NWORDS; ++i)
        word_list[i] = words[i];
    }
    /*!
     * \brief Returns a pointer to the word list.
     * \return A pointer to the word list.
     */
    const std::vector<std::string>& get_word_list() const
    {
      return word_list;
    }
    /*!
     * \brief Returns a pointer to the word map.
     * \return A pointer to the word map.
     */
    const std::unordered_map<epee::wipeable_string, uint32_t, WordHash, WordEqual>& get_word_map() const
    {
      return word_map;
    }
    /*!
     * \brief Returns a pointer to the trimmed word map.
     * \return A pointer to the trimmed word map.
     */
    const std::unordered_map<epee::wipeable_string, uint32_t, WordHash, WordEqual>& get_trimmed_word_map() const
    {
      return trimmed_word_map;
    }
    /*!
     * \brief Returns the name of the language.
     * \return Name of the language.
     */
    const std::string &get_language_name() const
    {
      return language_name;
    }
    /*!
     * \brief Returns the name of the language in English.
     * \return Name of the language.
     */
    const std::string &get_english_language_name() const
    {
      return english_language_name;
    }
    /*!
     * \brief Returns the number of unique starting characters to be used for matching.
     * \return Number of unique starting characters.
     */
    uint32_t get_unique_prefix_length() const
    {
      return unique_prefix_length;
    }
  };
}

#endif
