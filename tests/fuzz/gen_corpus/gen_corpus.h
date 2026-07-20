// Copyright (c) 2026, The Monero Project
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

#pragma once

#include <cstddef>

#define SEED_CORPUS(fuzzer_name, seed_corpus_name) \
    class gen_seed_corpus_ ## fuzzer_name ## _ ## seed_corpus_name : public ::fuzz::seed_corpus_source_single \
    { \
    protected: \
        const char * get_fuzzer_name() const override { return #fuzzer_name; }; \
        const char * get_seed_curpus_name() const override { return #seed_corpus_name; }; \
        void gen_corpus_single() override; \
    private: \
        [[maybe_unused]] static ::fuzz::registry::registry_entry_t registry_entry; \
    }; \
    ::fuzz::registry::registry_entry_t gen_seed_corpus_ ## fuzzer_name ## _ ## seed_corpus_name :: registry_entry = \
      ::fuzz::registry::register_seed_corpus_source(new gen_seed_corpus_ ## fuzzer_name ## _ ## seed_corpus_name()); \
    void gen_seed_corpus_ ## fuzzer_name ## _ ## seed_corpus_name :: gen_corpus_single() \

#define SET_SEED_CORPUS(seed_corpus_buf, seed_corpus_len) this->set_seed_corpus(seed_corpus_buf, seed_corpus_len)

namespace fuzz
{

/**
 * @brief Interface to consume seed corpora
 */
class seed_corpus_sink
{
public:
    virtual void send_seed_corpus(const char * fuzzer_name,
        const char * seed_corpus_name,
        const unsigned char * seed_corpus_buf,
        std::size_t seed_corpus_len) = 0;

    ~seed_corpus_sink() = default;
};

/**
 * @brief Interface to generate seed corpora
 */
class seed_corpus_source
{
public:
    virtual void gen_corpus(seed_corpus_sink &sink) = 0;

    ~seed_corpus_source() = default;
};

/**
 * @brief Abstract class which generates a single seed corpus, used by macros SEED_CORPUS() and SET_SEED_CORPUS()
 */
class seed_corpus_source_single: public seed_corpus_source
{
public:
    seed_corpus_source_single();

    void gen_corpus(seed_corpus_sink &sink) override;

protected:
    virtual const char * get_fuzzer_name() const = 0;
    virtual const char * get_seed_curpus_name() const = 0;
    virtual void gen_corpus_single() = 0;

    void set_seed_corpus(const unsigned char * seed_corpus_buf, std::size_t seed_corpus_len);

private:
    bool m_did_set_seed_corpus;
    seed_corpus_sink * m_sink;
};

namespace registry
{
struct registry_entry_t {};
/**
 * @brief Add seed corpus source to static registry
 * @param src Pointer to a seed corpus source (takes ownership)
 */
registry_entry_t register_seed_corpus_source(seed_corpus_source * src);

/**
 * @brief Run all sources passed to `register_seed_corpus_source()` against given `sink`
 */
void dispatch_all_registered_seed_corpus_sources(seed_corpus_sink &sink);
} //namespace registry

} //namespace fuzz
