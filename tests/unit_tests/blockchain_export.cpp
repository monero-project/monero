// Copyright (c) 2014-2024, The Monero Project
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

#include <fstream>
#include <stdexcept>
#include <string>

#include <boost/filesystem.hpp>
#include "gtest/gtest.h"

#include "blockchain_utilities/bootstrap_file.h"
#include "blockchain_utilities/blockchain_utilities.h"
#include "serialization/binary_utils.h"

namespace
{
  class bootstrap_file_writer final : public BootstrapFile
  {
  public:
    using BootstrapFile::close;
    using BootstrapFile::open_writer;

    void write_chunk(const std::string& data)
    {
      const uint32_t chunk_size = data.size();
      std::string encoded_size;
      ASSERT_TRUE(serialization::dump_binary(chunk_size, encoded_size));
      m_raw_data_file->write(encoded_size.data(), encoded_size.size());
      m_raw_data_file->write(data.data(), data.size());
      m_raw_data_file->flush();
      ASSERT_TRUE(m_raw_data_file->good());
    }
  };

  class blockchain_export_test : public testing::Test
  {
  protected:
    void SetUp() override
    {
      path = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("blockchain-%%%%%%%%.raw");
      initialize_file(0);
    }

    void initialize_file(uint64_t first_block)
    {
      boost::filesystem::remove(path);
      bootstrap_file_writer writer;
      uint64_t next_block = 0;
      ASSERT_TRUE(writer.open_writer(path, first_block, first_block, next_block));
      ASSERT_EQ(first_block, next_block);
      ASSERT_TRUE(writer.close());
    }

    void TearDown() override
    {
      boost::system::error_code ec;
      boost::filesystem::remove(path, ec);
    }

    void append_chunk(uint32_t chunk_size, const std::string& data)
    {
      std::string encoded_size;
      ASSERT_TRUE(serialization::dump_binary(chunk_size, encoded_size));
      ASSERT_EQ(sizeof(chunk_size), encoded_size.size());

      std::ofstream stream{path.string(), std::ios::binary | std::ios::app};
      ASSERT_TRUE(stream.good());
      stream.write(encoded_size.data(), encoded_size.size());
      stream.write(data.data(), data.size());
      ASSERT_TRUE(stream.good());
    }

    void append_partial_chunk_size(uint32_t chunk_size)
    {
      std::string encoded_size;
      ASSERT_TRUE(serialization::dump_binary(chunk_size, encoded_size));
      ASSERT_EQ(sizeof(chunk_size), encoded_size.size());

      std::ofstream stream{path.string(), std::ios::binary | std::ios::app};
      ASSERT_TRUE(stream.good());
      stream.write(encoded_size.data(), encoded_size.size() - 1);
      ASSERT_TRUE(stream.good());
    }

    void resume_with_chunk(const std::string& data, uint64_t expected_next_block)
    {
      bootstrap_file_writer writer;
      uint64_t next_block = 0;
      ASSERT_TRUE(writer.open_writer(path, 0, 0, next_block));
      ASSERT_EQ(expected_next_block, next_block);
      writer.write_chunk(data);
      ASSERT_TRUE(writer.close());
    }

    boost::filesystem::path path;
  };
}

TEST_F(blockchain_export_test, truncates_incomplete_chunk_data_before_appending)
{
  constexpr uint64_t complete_chunks = 12;
  for (uint64_t i = 0; i < complete_chunks; ++i)
    append_chunk(4, "data");
  const uint64_t complete_size = boost::filesystem::file_size(path);
  append_chunk(8, "bad");

  BootstrapFile reader;
  EXPECT_THROW(reader.count_blocks(path.string()), std::runtime_error);

  resume_with_chunk("more", complete_chunks);

  EXPECT_EQ(complete_size + sizeof(uint32_t) + 4, boost::filesystem::file_size(path));
  EXPECT_EQ(complete_chunks + 1, reader.count_blocks(path.string()));
}

TEST_F(blockchain_export_test, truncates_incomplete_chunk_size_before_appending)
{
  append_chunk(4, "data");
  const uint64_t complete_size = boost::filesystem::file_size(path);
  append_partial_chunk_size(8);

  BootstrapFile reader;
  EXPECT_THROW(reader.count_blocks(path.string()), std::runtime_error);

  resume_with_chunk("more", 1);

  EXPECT_EQ(complete_size + sizeof(uint32_t) + 4, boost::filesystem::file_size(path));
  EXPECT_EQ(2, reader.count_blocks(path.string()));
}

TEST_F(blockchain_export_test, preserves_nonzero_first_block_when_resuming)
{
  constexpr uint64_t first_block = 100;
  initialize_file(first_block);
  append_chunk(4, "data");
  append_partial_chunk_size(8);

  resume_with_chunk("more", first_block + 1);

  BootstrapFile reader;
  std::streampos start_pos;
  uint64_t seek_height = 0;
  uint64_t block_first = 0;
  EXPECT_EQ(2, reader.count_blocks(path.string(), start_pos, seek_height, block_first));
  EXPECT_EQ(first_block, block_first);
}

TEST_F(blockchain_export_test, rejects_invalid_chunk_size_without_truncating)
{
  append_chunk(4, "data");
  append_chunk(BUFFER_SIZE + 1, "");
  const uint64_t invalid_size = boost::filesystem::file_size(path);

  bootstrap_file_writer writer;
  uint64_t next_block = 0;
  EXPECT_THROW(writer.open_writer(path, 0, 0, next_block), std::runtime_error);

  EXPECT_EQ(invalid_size, boost::filesystem::file_size(path));
}
