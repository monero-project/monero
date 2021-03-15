// Copyright (c) 2014-2021, The Monero Project
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

#include "file_compression.h"

#ifdef WIN32
    #include <boost/iostreams/filter/zlib.hpp>
    #define COMPRESSOR zlib_compressor()
    #define DECOMPRESSOR zlib_decompressor()
    #define EXTENSION ".zip"
#else
    #include <boost/iostreams/filter/gzip.hpp>
    #define COMPRESSOR gzip_compressor()
    #define DECOMPRESSOR gzip_decompressor()
    #define EXTENSION ".gz"
#endif // WIN32
// Other possibilities to consider with newer Boost:
//#include <boost/iostreams/filter/lzma.hpp>
//#include <boost/iostreams/filter/zstd.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>

#include <fstream>
#include <string>

namespace epee
{
namespace file_compression
{
void decompress_file(const std::string & fileNameBase, std::ostream & outStream)
{
    namespace bio = boost::iostreams;
    std::ifstream file(fileNameBase + get_archive_extension(), std::ios_base::binary);
    bio::filtering_streambuf<bio::input> in;
    in.push(bio::DECOMPRESSOR);
    in.push(file);
    bio::copy(in, outStream);
}

void compress_file(const std::string & fileNameBase)
{
    namespace bio = boost::iostreams;
    std::ifstream inStream(fileNameBase, std::ios_base::in);
    std::ofstream outStream(fileNameBase + get_archive_extension(), std::ios_base::out);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(bio::COMPRESSOR);
    in.push(inStream);
    boost::iostreams::copy(in, outStream);
}

std::string get_archive_extension()
{
    return EXTENSION;
}
}
}
