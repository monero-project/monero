// Copyright (c) 2023, The Monero Project
// Copyright (c) 2021, tevador <tevador@gmail.com>
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#ifndef POLYSEED_HPP
#define POLYSEED_HPP

#include <polyseed/include/polyseed.h>
#include <polyseed/src/lang.h>
#include <vector>
#include <stdexcept>
#include <string>
#include "crypto/crypto.h"

namespace polyseed {

    class data;

    class language {
        public:
            language() : m_lang(nullptr) {}
            language(const language&) = default;
            language(const polyseed_lang* lang) : m_lang(lang) {}
            const char* name() const {
                return polyseed_get_lang_name(m_lang);
            }
            const char* name_en() const {
                return polyseed_get_lang_name_en(m_lang);
            }
            const char* separator() const {
                return m_lang->separator;
            }
            bool valid() const {
                return m_lang != nullptr;
            }

        const polyseed_lang* m_lang;
    private:

        friend class data;
    };

    const std::vector<language>& get_langs();
    const language& get_lang_by_name(const std::string& name);

    class error : public std::runtime_error {
        public:
            error(const char* msg, polyseed_status status)
                : std::runtime_error(msg), m_status(status)
            {
            }
            polyseed_status status() const {
                return m_status;
            }
        private:
            polyseed_status m_status;
    };

    using feature_type = unsigned int;

    inline int enable_features(feature_type features) {
        return polyseed_enable_features(features);
    }

    class data {
        public:
            data(const data&) = delete;
            data(polyseed_coin coin) : m_data(nullptr), m_coin(coin) {}
            ~data() {
                polyseed_free(m_data);
            }

            void create(feature_type features);

            void load(polyseed_storage storage);

            void load(const crypto::secret_key &key);

            language decode(const char* phrase);

            template<class str_type>
            void encode(const language& lang, str_type& str) const {
                check_valid();
                if (!lang.valid()) {
                    throw std::runtime_error("invalid language");
                }
                str.resize(POLYSEED_STR_SIZE);
                auto size = polyseed_encode(m_data, lang.m_lang, m_coin, &str[0]);
                str.resize(size);
            }

            void split(const language& lang, polyseed_phrase& words);

            void save(polyseed_storage storage) const {
                check_valid();
                polyseed_store(m_data, storage);
            }

            void save(void *storage) const {
              check_valid();
              polyseed_store(m_data, (uint8_t*)storage);
            }

            void crypt(const char* password) {
                check_valid();
                polyseed_crypt(m_data, password);
            }

            void keygen(void* ptr, size_t key_size) const {
                check_valid();
                polyseed_keygen(m_data, m_coin, key_size, (uint8_t*)ptr);
            }

            bool valid() const {
                return m_data != nullptr;
            }

            bool encrypted() const {
                check_valid();
                return polyseed_is_encrypted(m_data);
            }

            uint64_t birthday() const {
                check_valid();
                return polyseed_get_birthday(m_data);
            }

            bool has_feature(feature_type feature) const {
                check_valid();
                return polyseed_get_feature(m_data, feature) != 0;
            }
        private:
            void check_valid() const {
                if (m_data == nullptr) {
                    throw std::runtime_error("invalid object");
                }
            }
            void check_init() const;

            polyseed_data* m_data;
            polyseed_coin m_coin;
    };
}

#endif //POLYSEED_HPP
