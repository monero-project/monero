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

#ifndef _FILE_IO_UTILS_H_
#define _FILE_IO_UTILS_H_

#include <functional>
#include <iosfwd>
#include <string>
#include <ctime>
#include <cstdint>
#ifndef _WIN32
#include <sys/types.h>
#endif

namespace epee
{
namespace file_io_utils
{
    #ifdef _WIN32
    using file_mode = unsigned int;
    #else
    using file_mode = mode_t;
    #endif

    bool is_file_exist(const std::string& path);
namespace detail
{
    // The callback receives a file descriptor managed by save_fd_to_file and must not close it.
    // On Windows this is a CRT file descriptor wrapping a HANDLE.
    bool save_fd_to_file(const std::string& path_to_file, const std::function<bool(int)>& writer, file_mode create_mode = 0666);
}
    bool save_stream_to_file(const std::string& path_to_file, const std::function<bool(std::ostream&)>& writer, file_mode create_mode = 0666);
    bool save_string_to_file(const std::string& path_to_file, const std::string& str, file_mode create_mode = 0666);
    bool load_file_to_string(const std::string& path_to_file, std::string& target_str, size_t max_size = 1000000000);
}
}

#endif //_FILE_IO_UTILS_H_
