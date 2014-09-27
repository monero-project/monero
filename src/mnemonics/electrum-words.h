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
 * \file electrum-words.h
 * 
 * \brief Mnemonic seed generation and wallet restoration from them.
 * 
 * This file and its cpp file are for translating Electrum-style word lists
 * into their equivalent byte representations for cross-compatibility with
 * that method of "backing up" one's wallet keys.
 */

#include <string>
#include <cstdint>
#include <map>
#include "crypto/crypto.h"  // for declaration of crypto::secret_key

/*!
 * \namespace crypto
 */
namespace crypto
{
  /*!
   * \namespace ElectrumWords
   * 
   * \brief Mnemonic seed word generation and wallet restoration.
   */
  namespace ElectrumWords
  {
    /*!
     * \brief Called to initialize it work with a word list file.
     * \param language      Language of the word list file.
     * \param old_word_list Whether it is to use the old style word list file.
     */
    void init(const std::string &language, bool old_word_list=false);
    
    /*!
     * \brief Converts seed words to bytes (secret key).
     * \param  words String containing the words separated by spaces.
     * \param  dst   To put the secret key restored from the words.
     * \return       false if not a multiple of 3 words, or if word is not in the words list
     */
    bool words_to_bytes(const std::string& words, crypto::secret_key& dst);

    /*!
     * \brief Converts bytes (secret key) to seed words.
     * \param  src   Secret key
     * \param  words Space separated words get copied here.
     * \return       Whether it was successful or not. Unsuccessful if wrong key size.
     */
    bool bytes_to_words(const crypto::secret_key& src, std::string& words);

    /*!
     * \brief Gets a list of seed languages that are supported.
     * \param languages The list gets added to this.
     */
    void get_language_list(std::vector<std::string> &languages);

    /*!
     * \brief If the module is currenly using an old style word list.
     * \return Whether it is currenly using an old style word list.
     */
    bool get_is_old_style_mnemonics();
  }
}
