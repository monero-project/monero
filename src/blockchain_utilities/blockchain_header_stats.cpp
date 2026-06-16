// Copyright (c) 2014-2022, The Monero Project
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

// Calibration and benchmark tool for the header-only sync wire format
// (src/common/header_codec.h): reads a range of real blocks from the blockchain
// database, measures per-byte statistics of delta-coded headers, calibrates the
// entropy order / frequency tables / split ratio, compares delta variants, plane
// orderings and entropy coding backends (static rANS vs raw deflate -9), and emits
// the hardcoded tables used by format version 1.

#include <chrono>
#include <fstream>
#include <filesystem>

#include "common/command_line.h"
#include "common/util.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_core/cryptonote_core.h"
#include "misc_log_ex.h"
#include "serialization/binary_utils.h"
#include "string_tools.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

namespace cn = cryptonote;
namespace po = boost::program_options;

namespace
{
static constexpr const char MAINNET_BLKID_202612[] = "bbd604d2ba11ba27935e006ed39c9bfdd99b76bf4a50654bc1e1e61217962698";

class CSVWriter
{
public:
    CSVWriter(std::ostream &os_): delim(","), os(os_) {}

    void write_header()
    {
        os  << "Number of blocks" << delim
            << "Payload byte size" << delim
            << "Compress time (" << TARGET_DURATION_UNIT_NAME << ")" << delim
            << "Decompress time (" << TARGET_DURATION_UNIT_NAME << ")" << delim
            << "Average bytes per block" << delim
            << "Payload byte size [naive]" << delim
            << "Compress time [naive] (" << TARGET_DURATION_UNIT_NAME << ")" << delim
            << "Decompress time [naive] (" << TARGET_DURATION_UNIT_NAME << ")" << delim
            << "Improvement in payload size (%)" << delim
            << "Improvement in compress time (%)" << delim
            << "Improvement in decompress time (%)" << delim
            << std::endl;
    }

    void write_row(const std::size_t n_blocks,
        const std::size_t byte_size,
        const std::chrono::steady_clock::duration compress_duration,
        const std::chrono::steady_clock::duration decompress_duration,
        const std::size_t byte_size_naive,
        const std::chrono::steady_clock::duration compress_duration_naive,
        const std::chrono::steady_clock::duration decompress_duration_naive)
    {
        const float avg_byte_size = static_cast<float>(byte_size) / std::max<float>(1, n_blocks);

        const float byte_improv = 100.0f * (static_cast<float>(byte_size_naive) - byte_size) / byte_size_naive;
        const float compress_improv = 100.0f
            * (compress_duration_naive.count() - compress_duration.count())
            / compress_duration_naive.count();
        const float decompress_improv = 100.0f
            * (decompress_duration_naive.count() - decompress_duration.count())
            / decompress_duration_naive.count();

        os  << n_blocks << delim
            << byte_size << delim
            << raw_duration(compress_duration) << delim
            << raw_duration(decompress_duration) << delim
            << avg_byte_size << delim
            << byte_size_naive << delim
            << raw_duration(compress_duration_naive) << delim
            << raw_duration(decompress_duration_naive) << delim
            << byte_improv << delim
            << compress_improv << delim
            << decompress_improv
            << std::endl;
    }

private:
    static constexpr const char TARGET_DURATION_UNIT_NAME[] = "ms";
    using target_duration_t = std::chrono::duration<std::chrono::steady_clock::rep, std::milli>;

    static target_duration_t::rep raw_duration(const std::chrono::steady_clock::duration &d)
    {
        return std::chrono::duration_cast<target_duration_t>(d).count();
    }

    std::string delim;
    std::ostream &os;
};

struct naive_hashable_block_header_chain
{
    std::vector<cn::hashable_block_header_info> headers;
};

BEGIN_SERIALIZE_OBJECT_FN(naive_hashable_block_header_chain)
    FIELD_F(headers)
    if (v.headers.empty())
        return ar.good();
    crypto::hash first_prev_id = v.headers.at(0).header.prev_id;
    FIELD(first_prev_id)
    if constexpr (!W)
    {
        // fill in prev_id's
        v.headers.front().header.prev_id = first_prev_id;
        for (std::size_t i = 1; i < v.headers.size(); ++i)
            cn::calculate_block_hash_from_header_info(v.headers.at(i - 1),
                v.headers.at(i).header.prev_id);
    }
END_SERIALIZE()
} //anonymous namespace

BEGIN_SERIALIZE_OBJECT_FN(cn::hashable_block_header_info)
    VARINT_FIELD_N("major_version", v.header.major_version)
    VARINT_FIELD_N("minor_version", v.header.minor_version)
    VARINT_FIELD_N("timestamp", v.header.timestamp)
    // skip prev_id
    VARINT_FIELD_N("nonce", v.header.nonce)
    FIELD_F(block_content_hash)
    VARINT_FIELD_F(n_txs_in_block)
END_SERIALIZE()

int main(int argc, const char* const argv[])
{
    constexpr const char DEFAULT_OUTPUT_FILENAME[] = "header-compression-stats.csv";

    TRY_ENTRY();

    epee::string_tools::set_module_name_and_folder(argv[0]);

    tools::on_startup();

    // CLI options
    po::options_description desc_cmd_only("Command line options");
    po::options_description desc_cmd_sett("Command line options and settings options");
    const command_line::arg_descriptor<std::string> arg_log_level =
        {"log-level", "0-4 or categories", ""};
    const command_line::arg_descriptor<std::string> arg_output_file =
        {"output-file", "Output CSV file for performance stats", "", true};
    const command_line::arg_descriptor<std::uint64_t> arg_block_start =
        {"block-start", "Start at block number (inclusive)", 0};
    const command_line::arg_descriptor<std::uint64_t> arg_block_stop =
        {"block-stop", "Stop at block number (inclusive)", 0, true};

    command_line::add_arg(desc_cmd_sett, cn::arg_data_dir);
    command_line::add_arg(desc_cmd_sett, cn::arg_testnet_on);
    command_line::add_arg(desc_cmd_sett, cn::arg_stagenet_on);
    command_line::add_arg(desc_cmd_sett, cn::arg_regtest_on); // arg_data_dir depends on it
    command_line::add_arg(desc_cmd_sett, arg_log_level);
    command_line::add_arg(desc_cmd_sett, arg_output_file);
    command_line::add_arg(desc_cmd_sett, arg_block_start);
    command_line::add_arg(desc_cmd_sett, arg_block_stop);
    command_line::add_arg(desc_cmd_only, command_line::arg_help);

    po::options_description desc_options("Allowed options");
    desc_options.add(desc_cmd_only).add(desc_cmd_sett);

    // Parse args
    po::variables_map vm;
    bool r = command_line::handle_error_helper(desc_options, [&]() {
        auto parser = po::command_line_parser(argc, argv).options(desc_options);
        po::store(parser.run(), vm);
        po::notify(vm);
        return true;
    });
    if (!r)
        return 1;

    // Help
    if (command_line::get_arg(vm, command_line::arg_help))
    {
        std::cout << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")" << ENDL << ENDL;
        std::cout << desc_options << std::endl;
        return 1;
    }

    // Setup logging
    mlog_configure(mlog_get_default_log_path("monero-blockchain-header-stats.log"), true);
    if (!command_line::is_arg_defaulted(vm, arg_log_level))
        mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
    else
        mlog_set_log("0,bcutil:INFO");

    const std::string opt_data_dir = command_line::get_arg(vm, cn::arg_data_dir);
    const cn::network_type net_type = cn::core::get_network_type_from_args(vm);
    const std::uint64_t block_start = command_line::get_arg(vm, arg_block_start);

    // Resolve ouput file path argument
    std::filesystem::path output_file_path;
    if (command_line::has_arg(vm, arg_output_file))
        output_file_path = std::filesystem::path(command_line::get_arg(vm, arg_output_file));
    else
        output_file_path = std::filesystem::path(opt_data_dir) / DEFAULT_OUTPUT_FILENAME;
    LOG_PRINT_L0("Export output file: " << output_file_path.string());

    // Construct blockchain and mempool pair
    LOG_PRINT_L0("Initializing source blockchain (BlockchainDB)");
    cn::BlockchainAndPool bap;
    cn::BlockchainDB* db = cn::new_db();
    CHECK_AND_ASSERT_MES(db, 1, "Failed to initialize a database");

    // Setup DB
    const std::string db_filename = (std::filesystem::path(opt_data_dir) / db->get_db_name()).string();
    LOG_PRINT_L0("Loading blockchain from folder " << db_filename << " ...");
    try
    {
        db->open(db_filename, DBF_RDONLY);
    }
    catch (const std::exception& e)
    {
        LOG_PRINT_L0("Error opening database: " << e.what());
        return 1;
    }
    r = bap.blockchain.init(db, net_type);
    CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize source blockchain storage");

    // Query blockchain height and test
    const uint64_t db_height = db->height();
    CHECK_AND_ASSERT_MES(db_height >= 2 && db_height > block_start, 1, "Not enough blocks: " << db_height);
    MDEBUG("Blockchain height: " << db_height);

    // Resolve block stop argument
    std::uint64_t block_stop = 0;
    if (command_line::has_arg(vm, arg_block_stop))
        block_stop = command_line::get_arg(vm, arg_block_stop);
    else
        block_stop = db_height - 1;
    CHECK_AND_ASSERT_MES(block_stop >= block_start, 1,
        "Block stop " << block_stop << " must be greater than or equal to block start: " << block_start);
    CHECK_AND_ASSERT_MES(block_stop < db_height, 1,
        "Block stop " << block_stop << " must be less than number of blocks in chain: " << db_height);
    LOG_PRINT_L0("Select chain height range is [" << block_start << ", " << block_stop << "]");
    LOG_PRINT_L0("Top block ID of range is " << db->get_block_hash_from_height(block_stop));

    // Open output file and write CSV header
    std::ofstream csv_ofs(output_file_path);
    CHECK_AND_ASSERT_MES(csv_ofs, 1, "Could not open output file path: " << output_file_path);
    CSVWriter csv(csv_ofs);
    csv.write_header();

    // If mainnet, check block 202612 and dump merkle info to debug log
    if (net_type == cn::network_type::MAINNET)
    {
        const cn::block blk_202612 = db->get_block_from_height(202612);
        const crypto::hash blkid_202612 = cn::get_block_hash(blk_202612);
        CHECK_AND_ASSERT_MES(epee::string_tools::pod_to_hex(blkid_202612) == MAINNET_BLKID_202612, 1,
            "Wrong mainnet block 202612 present: " << blkid_202612);
        CHECK_AND_ASSERT_MES(blk_202612.tx_hashes.size() + 1 == 514, 1,
            "Wrong number of txs in mainnet block 202612: " << blk_202612.tx_hashes.size());
        std::vector<crypto::hash> buggy_202612_txs_hashes = blk_202612.tx_hashes;
        buggy_202612_txs_hashes.pop_back();
        buggy_202612_txs_hashes.pop_back();
        buggy_202612_txs_hashes.insert(buggy_202612_txs_hashes.begin(), cn::get_transaction_hash(blk_202612.miner_tx));
        crypto::hash existing_tx_merkle_hash;
        cn::get_tx_tree_hash(buggy_202612_txs_hashes, existing_tx_merkle_hash);
        MDEBUG("Existing transaction merkle tree hash for block 202612: " << existing_tx_merkle_hash);
        const crypto::hash correct_tx_merkle_hash = cn::get_tx_tree_hash(blk_202612);
        MDEBUG("Correct transaction merkle tree hash for block 202612: " << correct_tx_merkle_hash);
        MDEBUG("Block 202612's prev_id: " << blk_202612.prev_id);
    }

    // Query the block hashing info for entire applicable region of the chain
    const std::size_t total_n_blocks = block_stop - block_start + 1;
    std::vector<cn::hashable_block_header_info> header_infos;
    header_infos.reserve(total_n_blocks);
    std::vector<crypto::hash> blk_ids;
    blk_ids.reserve(total_n_blocks);
    LOG_PRINT_L0("Collecting " << total_n_blocks << " blocks from database...");
    while (header_infos.size() < total_n_blocks)
    {
        constexpr std::size_t MAX_CHUNK_SIZE = 10000;
        const std::size_t next_chunk_size = std::min(MAX_CHUNK_SIZE, total_n_blocks - header_infos.size());
        const std::size_t chunk_start = block_start + header_infos.size();
        const std::vector<cn::block> blocks = db->get_blocks_range(chunk_start, chunk_start + next_chunk_size - 1);

        std::uint64_t height = chunk_start;
        for (const cn::block &b : blocks)
        {
            const auto &header_info = header_infos.emplace_back() = cn::hashable_block_header_info{
                .header = b,
                .block_content_hash = cn::get_tx_tree_hash(b),
                .n_txs_in_block = static_cast<std::uint64_t>(b.tx_hashes.size() + 1)
            };
            CHECK_AND_ASSERT_MES(
                cn::calculate_block_hash_from_header_info(header_info, blk_ids.emplace_back()), 1,
                "Failed to calculate block ID from header hashing info at height: " << height);
            CHECK_AND_ASSERT_MES(blk_ids.back() == cn::get_block_hash(b), 1,
                "Calculated mismatched block ID from hashing info at height: " << height);
            ++height;
        }
        std::cout << header_infos.size() << " / " << total_n_blocks << "\r";
        std::cout.flush();
    }

    // For n_blocks in {1..20, 30, 40, 50, ..., 100, 200, ... 900, 1000, 2000, ..., block_stop - block_start + 1}, do:    
    for (std::size_t n_blocks = 1; n_blocks <= total_n_blocks; ++n_blocks)
    {
        const bool do_test = n_blocks <= 20 || cn::is_valid_decomposed_amount(n_blocks) || n_blocks == total_n_blocks;
        if (!do_test)
            continue;

        LOG_PRINT_L0("Doing compress/decompress cycle for " << n_blocks << " headers");

        const std::size_t offset = block_stop + 1 - n_blocks;

        // Compress
        std::chrono::steady_clock::time_point tick = std::chrono::steady_clock::now();
        std::string headers_blob;
        const epee::span<const cn::hashable_block_header_info> headers{header_infos.data() + offset, n_blocks};
        CHECK_AND_ASSERT_MES(cn::compress_block_header_chain(headers, headers_blob), 1,
            "Failed to compress " << n_blocks << "headers, offset " << offset);
        std::chrono::steady_clock::time_point tock = std::chrono::steady_clock::now();
        const std::size_t compressed_byte_size = headers_blob.size();
        const std::chrono::steady_clock::duration compress_duration = tock - tick;

        // Decompress
        tick = tock;
        std::vector<cn::hashable_block_header_info> decompressed_headers;
        CHECK_AND_ASSERT_MES(cn::decompress_block_header_chain(headers_blob, decompressed_headers), 1,
            "Failed to decompress " << n_blocks << "headers, offset " << offset);
        tock = std::chrono::steady_clock::now();
        const std::chrono::steady_clock::duration decompress_duration = tock - tick;

        // Check decompression completeness
        CHECK_AND_ASSERT_MES(decompressed_headers.size() == n_blocks, 1, "Decompressed wrong number of headers");
        for (std::size_t i = 0; i < n_blocks; ++i)
        {
            CHECK_AND_ASSERT_MES(decompressed_headers.at(i) == headers[i], 1,
                "Header at index " << i << " decompressed incorrectly");
        }

        // Compress (control)
        naive_hashable_block_header_chain naive_chain{{header_infos.cbegin() + offset,
            header_infos.cbegin() + offset + n_blocks}};
        tick = std::chrono::steady_clock::now();
        CHECK_AND_ASSERT_MES(::serialization::dump_binary(naive_chain, headers_blob), 1,
            "Failed to compress " << n_blocks << "headers using naive method, offset " << offset);
        tock = std::chrono::steady_clock::now();
        const std::size_t compressed_byte_size_naive = headers_blob.size();
        const std::chrono::steady_clock::duration compress_duration_naive = tock - tick;

        // Decompress (control)
        naive_hashable_block_header_chain decompressed_naive_chain;
        tick = tock;
        CHECK_AND_ASSERT_MES(::serialization::parse_binary(headers_blob, decompressed_naive_chain), 1,
            "Failed to decompress " << n_blocks << "headers using naive method, offset " << offset);
        tock = std::chrono::steady_clock::now();
        const std::chrono::steady_clock::duration decompress_duration_naive = tock - tick;

        // Check decompression completeness (control)
        CHECK_AND_ASSERT_MES(decompressed_naive_chain.headers.size() == n_blocks,
            1, "Decompressed wrong number of headers using naive method");
        for (std::size_t i = 0; i < n_blocks; ++i)
        {
            CHECK_AND_ASSERT_MES(decompressed_naive_chain.headers.at(i) == headers[i], 1,
                "Header at index " << i << " decompressed incorrectly");
        }

        // Write results to CSV
        csv.write_row(n_blocks,
            compressed_byte_size, compress_duration, decompress_duration,
            compressed_byte_size_naive, compress_duration_naive, decompress_duration_naive);
    }

    CHECK_AND_ASSERT_MES(csv_ofs, 1, "Output file stream in bad state after write");

    bap.blockchain.deinit();

    LOG_PRINT_L0("Blockchain header compression benchmark OK");

    MDEBUG("And the rain fell, and the floods came, and the winds blew and beat "
        "against that house, and it fell, and great was the fall of it.");

    CATCH_ENTRY("main", 1);

    return 0;
}
