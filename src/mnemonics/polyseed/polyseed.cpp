// Copyright (c) 2026, The Monero Project
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

#include "polyseed.hpp"
#include "pbkdf2.h"

#include <sodium/core.h>
#include <sodium/utils.h>
#include <sodium/randombytes.h>
#include <utf8proc.h>

#include <cstring>
#include <algorithm>
#include <array>

#include "cryptonote_basic/cryptonote_format_utils.h"

namespace polyseed {

    inline size_t utf8_norm(const char* str, polyseed_str norm, utf8proc_option_t options) {
      utf8proc_int32_t buffer[POLYSEED_STR_SIZE];
      utf8proc_ssize_t result;

      result = utf8proc_decompose(reinterpret_cast<const uint8_t*>(str), 0, buffer, POLYSEED_STR_SIZE, options);
      if (result < 0 || result > (POLYSEED_STR_SIZE - 1)) {
        throw std::runtime_error("Unicode normalization failed");
      }

      result = utf8proc_reencode(buffer, result, options);
      if (result < 0 || result > (POLYSEED_STR_SIZE - 1)) {
        throw std::runtime_error("Unicode normalization failed");
      }

      strcpy(norm, reinterpret_cast<const char*>(buffer));
      sodium_memzero(buffer, POLYSEED_STR_SIZE);
      return result;
    }

    static size_t utf8_nfc(const char* str, polyseed_str norm) {
        return utf8_norm(str, norm, (utf8proc_option_t)(UTF8PROC_NULLTERM | UTF8PROC_STABLE | UTF8PROC_COMPOSE | UTF8PROC_STRIPNA));
    }

    static size_t utf8_nfkd(const char* str, polyseed_str norm) {
        return utf8_norm(str, norm, (utf8proc_option_t)(UTF8PROC_NULLTERM | UTF8PROC_STABLE | UTF8PROC_DECOMPOSE | UTF8PROC_COMPAT | UTF8PROC_STRIPNA));
    }

    struct dependency {
        dependency();
        std::vector<language> languages;
    };

    static dependency deps;

    dependency::dependency() {
        if (sodium_init() == -1) {
            throw std::runtime_error("sodium_init failed");
        }

        polyseed_dependency pd;
        pd.randbytes = &randombytes_buf;
        pd.pbkdf2_sha256 = &crypto_pbkdf2_sha256;
        pd.memzero = &sodium_memzero;
        pd.u8_nfc = &utf8_nfc;
        pd.u8_nfkd = &utf8_nfkd;
        pd.time = nullptr;
        pd.alloc = nullptr;
        pd.free = nullptr;

        polyseed_inject(&pd);

        for (int i = 0; i < polyseed_get_num_langs(); ++i) {
            languages.push_back(language(polyseed_get_lang(i)));
        }
    }

    static language invalid_lang;

    const std::vector<language>& get_langs() {
        return deps.languages;
    }

    const language& get_lang_by_name(const std::string& name) {
        for (auto& lang : deps.languages) {
            if (name == lang.name_en()) {
                return lang;
            }
            if (name == lang.name()) {
                return lang;
            }
        }
        return invalid_lang;
    }

    inline void data::check_init() const {
        if (valid()) {
            throw std::runtime_error("already initialized");
        }
    }

    void data::check_valid() const {
        if (m_data == nullptr) {
            throw std::runtime_error("invalid object");
        }
    }

    static std::array<const char*, 8> error_desc = {
        "Success",
        "Wrong number of words in the phrase",
        "Unknown language or unsupported words",
        "Checksum mismatch",
        "Unsupported seed features",
        "Invalid seed format",
        "Memory allocation failure",
        "Phrase matches more than one language"
    };

    static error get_error(polyseed_status status) {
        if (status > 0 && status < sizeof(error_desc) / sizeof(const char*)) {
            return error(error_desc[(int)status], status);
        }
        return error("Unknown error", status);
    }

    void data::create(feature_type features) {
        check_init();
        auto status = polyseed_create(features, &m_data);
        if (status != POLYSEED_OK) {
            throw get_error(status);
        }
    }

    // Create a new random seed that is not ambiguous for sure if encoded in the given language
    void data::create(feature_type features, const language& lang) {
        for (; ;) {
            create(features);
            if (!is_ambiguous(lang)) {
                break;
            }
            polyseed_free(m_data);
            m_data = NULL;
        }
    }

    void data::load(polyseed_storage storage) {
        check_init();
        auto status = polyseed_load(storage, &m_data);
        if (status != POLYSEED_OK) {
            throw get_error(status);
        }
    }

    void data::load(const crypto::secret_key &key) {
        polyseed_storage d;
        memcpy(&d, &key.data, 32);
        auto status = polyseed_load(d, &m_data);
        if (status != POLYSEED_OK) {
            throw get_error(status);
        }
    }

    language data::decode(const char* phrase) {
        check_init();
        const polyseed_lang* lang;
        auto status = polyseed_decode(phrase, m_coin, &lang, &m_data);
        if (status != POLYSEED_OK) {
            throw get_error(status);
        }
        return language(lang);
    }

    // Check whether the seed, encoded in the given language, is ambiguous i.e. whether the
    // resulting seed would be a valid seed in more than one language. Tests have shown that
    // this is rather unlikely, but not astronomically so.
    bool data::is_ambiguous(const language& lang) {
        check_valid();
        epee::wipeable_string phrase;
        encode(lang, phrase);
        polyseed::data check_polyseed(POLYSEED_MONERO);
        bool is_ambiguous = false;
        try {
            const language& check_language = check_polyseed.decode(phrase.data());
            is_ambiguous = check_language.m_lang != lang.m_lang;
        }
        catch (const polyseed::error& error) {
            if (error.status() == POLYSEED_ERR_MULT_LANG) {
                is_ambiguous = true;
            }
            else {
                throw(error);
            }
        }
        return is_ambiguous;
    }

    void data::save(polyseed_storage storage) const {
        check_valid();
        polyseed_store(m_data, storage);
    }

    void data::save(void *storage) const {
        check_valid();
        polyseed_store(m_data, (uint8_t*)storage);
    }

    void data::crypt(const char* password) {
        check_valid();
        polyseed_crypt(m_data, password);
    }

    void data::keygen(void* ptr, size_t key_size) const {
        check_valid();
        polyseed_keygen(m_data, m_coin, key_size, (uint8_t*)ptr);
    }

    // Generate a key to serve as the spend secret key, given Polyseed data and an optional passphrase / seed offset;
    //
    // We use the following restore strategy regarding passphrase: If the seed is encrypted in the sense of the
    // Polyseed 'Encrypted' feature bit, we take the passphrase to be the password / decryption key and decrypt.
    // If the Polyseed is not encrypted, we use the passphrase to do the usual "seed offsetting" as we do it
    // already for a long time in the core software with secret keys defined through 25 word legacy seeds.
    //
    // Although strictly speaking seed offsetting is not part of the Polyseed spec, various third-party wallet
    // apps do it already in this way for quite some time, and there are zero technical problems with it,
    // thus we follow this as some sort of "de facto standard".
    //
    // We support encrypted Polyseeds here because an important third-party wallet app does NOT do seed
    // offsetting if the user specifies a passphrase at new wallet creation, but encrypts the seed with it,
    // and we want to be able to restore wallets from those seeds. However, we don't ever produce encrypted
    // Polyseeds ourselves in the Monero core code and always offset the seed with any giving passphrase.
    //
    // Said third-party wallet app uses, as far as RESTORING from Polyseed with optional passphrase is
    // concerned, exactly the same strategy as implemented here.
    crypto::secret_key data::generate_secret_key(const epee::wipeable_string &passphrase) const
    {
        bool is_encrypted = encrypted();
        bool has_passphrase = !passphrase.empty();

        polyseed_storage storage;
        save(storage);
        polyseed::data seed_copy(POLYSEED_MONERO);
        seed_copy.load(storage);

        if (is_encrypted) {
        if (!has_passphrase) {
            throw std::runtime_error("encrypted polyseed needs a passphrase");
        }
        epee::wipeable_string zero_terminated_passphrase(passphrase);
        zero_terminated_passphrase.push_back('\0');
        seed_copy.crypt(zero_terminated_passphrase.data());
        }
        crypto::secret_key secret_key;
        seed_copy.keygen(&secret_key, sizeof(secret_key));

        if (!is_encrypted && has_passphrase) {
        secret_key = cryptonote::decrypt_key(secret_key, passphrase);
        }
        return secret_key;
    }

    bool data::valid() const {
        return m_data != nullptr;
    }

    bool data::encrypted() const {
        check_valid();
        return polyseed_is_encrypted(m_data);
    }

    uint64_t data::birthday() const {
        check_valid();
        return polyseed_get_birthday(m_data);
    }

    bool data::has_feature(feature_type feature) const {
        check_valid();
        return polyseed_get_feature(m_data, feature) != 0;
    }

}
