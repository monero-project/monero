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

#include "common/command_line.h"
#include "common/util.h"
#include "hex.h"
#include "misc_log_ex.h"
#include "string_tools.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <openssl/evp.h>
#include <string>

namespace po = boost::program_options;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "fuzz"

namespace
{
std::string sha1_hex(const void *data, size_t len)
{
    unsigned char md[20];
    unsigned int md_len = sizeof(md);
    EVP_Digest(data, len, md, &md_len, EVP_sha1(), nullptr);

    return epee::to_hex::string(md);
}

class FileSeedCorpusSink: public fuzz::seed_corpus_sink
{
public:
    FileSeedCorpusSink(std::filesystem::path fuzz_data_dir, bool overwrite_existing):
        m_fuzz_data_dir(fuzz_data_dir),
        m_overwrite_existing(overwrite_existing)
    {
        if (!std::filesystem::exists(m_fuzz_data_dir))
            std::filesystem::create_directories(m_fuzz_data_dir);
    }

    void send_seed_corpus(const char * fuzzer_name,
        const char * seed_corpus_name,
        const unsigned char * seed_corpus_buf,
        std::size_t seed_corpus_len) override
    {
        std::filesystem::create_directories(m_fuzz_data_dir / fuzzer_name);

        const std::filesystem::path seed_corpus_path = m_fuzz_data_dir / fuzzer_name / seed_corpus_name;
        if (!m_overwrite_existing && std::filesystem::exists(seed_corpus_path))
            throw std::runtime_error(std::string("File ") + seed_corpus_path.string() + " already exists");

        std::ofstream ofs(seed_corpus_path, std::ios_base::binary | std::ios_base::out);
        ofs.write(reinterpret_cast<const char*>(seed_corpus_buf), seed_corpus_len);
        if (!ofs.good())
            throw std::runtime_error("Could not write seed corpus to file: " + seed_corpus_path.string());

        const std::string corpus_full_name = std::string(fuzzer_name) + "/" + seed_corpus_name;
        m_generated_sha1s.emplace(corpus_full_name, sha1_hex(seed_corpus_buf, seed_corpus_len));
    }

    void dump_sha1s() const
    {
        size_t longest_name = 0;
        for (const auto &p : m_generated_sha1s)
            longest_name = std::max(longest_name, p.first.size());

        MINFO("Seed corpus SHA1 hashes:");
        for (const auto &p : m_generated_sha1s)
        {
            std::string padded_name = p.first;
            padded_name.resize(longest_name, ' ');
            MINFO(padded_name << " - " << p.second);
        }
    }

    ~FileSeedCorpusSink()
    {
        try { dump_sha1s(); }
        catch (...) {}
    }

private:
    std::filesystem::path m_fuzz_data_dir;
    bool m_overwrite_existing;
    std::map<std::string, std::string> m_generated_sha1s;
};

} //anonymous namespace

int main(int argc, const char * const argv[])
{
    tools::on_startup();
    epee::string_tools::set_module_name_and_folder(argv[0]);
    mlog_configure(mlog_get_default_log_path("gen_corpus.log"), true);

    // the default test data directory is ../../data (relative to the executable's directory)
    const auto default_test_data_dir = std::filesystem::canonical(argv[0]).parent_path()
        .parent_path().parent_path() / "data" / "fuzz";

    po::options_description desc_options("Command line options");
    const command_line::arg_descriptor<std::string> arg_data_dir = {
        "data-dir",
        "Data files directory",
        default_test_data_dir.string()};
    const command_line::arg_descriptor<std::string> arg_log_level = {
        "log-level",
        "0-4 or categories",
        ""};
    const command_line::arg_descriptor<bool> arg_force = {
        "force",
        "Overwrite existing corpus files",
        false
    };
    command_line::add_arg(desc_options, arg_data_dir);
    command_line::add_arg(desc_options, arg_log_level);
    command_line::add_arg(desc_options, arg_force);

    po::variables_map vm;
    bool r = command_line::handle_error_helper(desc_options, [&]()
    {
        po::store(po::parse_command_line(argc, argv, desc_options), vm);
        po::notify(vm);
        return true;
    });
    if (!r)
        return 1;

    // set the log level
    if (!command_line::is_arg_defaulted(vm, arg_log_level))
        mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
    else
        mlog_set_log(MONERO_DEFAULT_LOG_CATEGORY ":INFO");

    // get the data dir
    const std::string data_dir = command_line::get_arg(vm, arg_data_dir);
    MINFO("Writing fuzzing seed corpus files to: " << data_dir);
    const bool overwrite_existing = command_line::get_arg(vm, arg_force);

    // run seed corpus generation
    FileSeedCorpusSink seed_corpus_sink(data_dir, overwrite_existing);
    ::fuzz::registry::dispatch_all_registered_seed_corpus_sources(seed_corpus_sink);

    return 0;
}
