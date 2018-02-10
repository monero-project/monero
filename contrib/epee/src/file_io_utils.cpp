// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts copyright (c) 2018 the Monero Project

#include <ostream>
#include "misc_log_ex.h"
#include "file_io_utils.h"

namespace epee
{
namespace file_io_utils
{
  //----------------------------------------------------------------------------
  void wipe_file(const std::string &path_to_file)
  {
    try
    {
      std::ofstream fstream;
      fstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      fstream.open(path_to_file, std::ios_base::binary | std::ios_base::out | std::ios_base::in | std::ios::ate);
      uint64_t size = fstream.tellp();
      fstream.seekp(0);
      std::unique_ptr<char[]> zero(new char[4096]);
      memset(zero.get(), 0, 4096);
      while (size > 0)
      {
        const size_t chunk_size = size > 4096 ? 4096 : size;
        fstream.write(zero.get(), chunk_size);
        size -= chunk_size;
      }
      fstream.flush();
      fstream.close();
      if (!boost::filesystem::remove(path_to_file))
        throw std::runtime_error("boost::filesystem::remove call failed");
    }
    catch (const std::exception &e)
    {
      MERROR("Failed to wipe file " << path_to_file << ": " << e.what());
      throw;
    }
  }
  //----------------------------------------------------------------------------
}
}

