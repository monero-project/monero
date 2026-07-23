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

#include "gen_corpus.h"

#include "misc_log_ex.h"
#include "scope_guard.h"

#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "fuzz"

namespace fuzz
{
namespace
{
std::list<std::unique_ptr<seed_corpus_source>> registered_seed_corpus_sources;
std::mutex registry_mutex;
} //anonymous namespace

seed_corpus_source_single::seed_corpus_source_single()
    : m_did_set_seed_corpus(false), m_sink(nullptr)
{}

void seed_corpus_source_single::gen_corpus(seed_corpus_sink &sink)
{
    if (this->m_did_set_seed_corpus || this->m_sink)
        throw std::logic_error("seed_corpus_source_single::gen_corpus is not re-entrant");

    const epee::scope_guard clear_on_exit([this](){
        this->m_did_set_seed_corpus = false;
        this->m_sink = nullptr;
    });

    MINFO("Generating corpus " << this->get_fuzzer_name() << "/" << this->get_seed_curpus_name() << "...");

    this->m_sink = &sink;
    this->gen_corpus_single();

    if (!this->m_did_set_seed_corpus)
        throw std::logic_error("Seed corpus was never set in body");
}

void seed_corpus_source_single::set_seed_corpus(const unsigned char * seed_corpus_buf, std::size_t seed_corpus_len)
{
    if (!this->m_sink)
        throw std::logic_error("seed_corpus_source_single::set_seed_corpus called while not generating");
    else if (this->m_did_set_seed_corpus)
        throw std::logic_error("seed_corpus_source_single::set_seed_corpus called more than once per generation");

    this->m_sink->send_seed_corpus(this->get_fuzzer_name(),
        this->get_seed_curpus_name(),
        seed_corpus_buf,
        seed_corpus_len);
    this->m_did_set_seed_corpus = true;
}

namespace registry
{
registry_entry_t register_seed_corpus_source(seed_corpus_source * src)
{
    const std::lock_guard registry_lock(registry_mutex);
    registered_seed_corpus_sources.emplace_back(src);
    return {};
}

void dispatch_all_registered_seed_corpus_sources(seed_corpus_sink &sink)
{
    const std::lock_guard registry_lock(registry_mutex);
    for (auto &src : registered_seed_corpus_sources)
        src->gen_corpus(sink);
}

} //namespace registry

} //namespace fuzz
