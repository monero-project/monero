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

#include "file_io_utils.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <fstream>
#include <ostream>
#include <streambuf>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include "string_tools.h"
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// On Windows there is a problem with non-ASCII characters in path and file names
// as far as support by the standard components used is concerned:

// The various file stream classes, e.g. std::ifstream and std::ofstream, are
// part of the GNU C++ Library / libstdc++. On the most basic level they use the
// fopen() call as defined / made accessible to programs compiled within MSYS2
// by the stdio.h header file maintained by the MinGW project.

// The critical point: The implementation of fopen() is part of MSVCRT, the
// Microsoft Visual C/C++ Runtime Library, and this method does NOT offer any
// Unicode support.

// Monero code that would want to continue to use the normal file stream classes
// but WITH Unicode support could therefore not solve this problem on its own,
// but 2 different projects from 2 different maintaining groups would need changes
// in this particular direction - something probably difficult to achieve and
// with a long time to wait until all new versions / releases arrive.

// Implemented solution approach: Circumvent the problem by stopping to use std
// file stream classes on Windows and directly use Unicode-capable WIN32 API
// calls. Most of the code doing so is concentrated in this header file here.

namespace epee
{
namespace file_io_utils
{
	namespace
	{
		bool write_all(const int fd, const char* data, size_t size)
		{
			while (size != 0)
			{
				const size_t chunk = std::min<size_t>(size, INT_MAX);
#ifdef _WIN32
				const int written = _write(fd, data, static_cast<unsigned int>(chunk));
#else
				const ssize_t written = write(fd, data, chunk);
#endif
				if (written < 0)
				{
					if (errno == EINTR)
						continue;
					return false;
				}
				if (written == 0)
				{
					errno = EIO;
					return false;
				}
				data += written;
				size -= static_cast<size_t>(written);
			}
			return true;
		}

		class file_streambuf final : public std::streambuf
		{
		public:
			explicit file_streambuf(const int fd)
				: fd_(fd)
			{
				setp(buffer_.data(), buffer_.data() + buffer_.size());
			}

		protected:
			int_type overflow(const int_type ch) override
			{
				if (!flush())
					return traits_type::eof();
				if (!traits_type::eq_int_type(ch, traits_type::eof()))
				{
					*pptr() = traits_type::to_char_type(ch);
					pbump(1);
				}
				return traits_type::not_eof(ch);
			}

			int sync() override
			{
				return flush() ? 0 : -1;
			}

		private:
			bool flush()
			{
				const std::ptrdiff_t size = pptr() - pbase();
				if (size <= 0)
					return true;
				pbump(-static_cast<int>(size));
				return write_all(fd_, buffer_.data(), static_cast<size_t>(size));
			}

			const int fd_;
			std::array<char, 16384> buffer_;
		};
	}
 
	bool is_file_exist(const std::string& path)
	{
		boost::filesystem::path p(path);
		return boost::filesystem::exists(p);
	}


namespace detail
{
	bool save_fd_to_file(const std::string& path_to_file, const std::function<bool(int)>& writer, file_mode create_mode)
	{
#ifdef _WIN32
		(void)create_mode;
		std::wstring wide_path;
		try { wide_path = string_tools::utf8_to_utf16(path_to_file); } catch (...) { return false; }
		HANDLE file_handle = CreateFileW(wide_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file_handle == INVALID_HANDLE_VALUE)
			return false;
		const int file_fd = _open_osfhandle(reinterpret_cast<intptr_t>(file_handle), _O_WRONLY | _O_BINARY);
		if (file_fd < 0)
		{
			const int error = errno;
			CloseHandle(file_handle);
			errno = error;
			return false;
		}
		// The CRT descriptor now owns file_handle; _close(file_fd) closes both.
#else
		errno = 0;
		struct stat st;
		int result;
		do
		{
			result = lstat(path_to_file.c_str(), &st);
		} while (result != 0 && errno == EINTR);
		if (result == 0)
		{
			if (S_ISLNK(st.st_mode) || !S_ISREG(st.st_mode))
				return false;
		}
		else if (errno != ENOENT)
		{
			return false;
		}

#ifdef O_NOFOLLOW
		const int flags = O_WRONLY | O_CREAT | O_NOFOLLOW;
#else
		const int flags = O_WRONLY | O_CREAT;
#endif
		int file_fd;
		do
		{
			file_fd = open(path_to_file.c_str(), flags, create_mode);
		} while (file_fd < 0 && errno == EINTR);
		if (file_fd < 0)
			return false;

		do
		{
			result = fstat(file_fd, &st);
		} while (result != 0 && errno == EINTR);
		if (result != 0 || !S_ISREG(st.st_mode))
		{
			const int error = result != 0 ? errno : EINVAL;
			close(file_fd);
			errno = error;
			return false;
		}

		do
		{
			result = ftruncate(file_fd, 0);
		} while (result != 0 && errno == EINTR);
		if (result != 0)
		{
			const int error = errno;
			close(file_fd);
			errno = error;
			return false;
		}
#endif

		errno = 0;
		bool writer_result = false;
		try
		{
			writer_result = writer(file_fd);
		}
		catch (...)
		{
			if (errno == 0)
				errno = EIO;
		}
		const int writer_error = errno;
#ifdef _WIN32
		const int close_result = _close(file_fd);
#else
		const int close_result = close(file_fd);
#endif
		const int close_error = errno;
		if (!writer_result)
			errno = writer_error ? writer_error : (close_result != 0 ? close_error : EIO);
		else if (close_result != 0)
			errno = close_error;
		return writer_result && close_result == 0;
	}
}


	bool save_stream_to_file(const std::string& path_to_file, const std::function<bool(std::ostream&)>& writer, file_mode create_mode)
	{
		return detail::save_fd_to_file(path_to_file, [&writer](const int fd)
		{
			file_streambuf buffer{fd};
			std::ostream stream{&buffer};
			const bool result = writer(stream);
			if (!result)
				return false;
			stream.flush();
			return stream.good();
		}, create_mode);
	}


	bool save_string_to_file(const std::string& path_to_file, const std::string& str, file_mode create_mode)
	{
		return detail::save_fd_to_file(path_to_file, [&str](const int fd)
		{
			return write_all(fd, str.data(), str.size());
		}, create_mode);
	}


	bool load_file_to_string(const std::string& path_to_file, std::string& target_str, size_t max_size)
	{
#ifdef _WIN32
                std::wstring wide_path;
                try { wide_path = string_tools::utf8_to_utf16(path_to_file); } catch (...) { return false; }
                HANDLE file_handle = CreateFileW(wide_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (file_handle == INVALID_HANDLE_VALUE)
                    return false;
                DWORD file_size = GetFileSize(file_handle, NULL);
                if ((file_size == INVALID_FILE_SIZE) || (uint64_t)file_size > (uint64_t)max_size) {
                    CloseHandle(file_handle);
                    return false;
                }
                target_str.resize(file_size);
                DWORD bytes_read;
                BOOL result = ReadFile(file_handle, &target_str[0], file_size, &bytes_read, NULL);
                CloseHandle(file_handle);
                if (bytes_read != file_size)
                    result = FALSE;
                return result;
#else
		try
		{
			std::ifstream fstream;
			fstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			fstream.open(path_to_file, std::ios_base::binary | std::ios_base::in | std::ios::ate);

			std::ifstream::pos_type file_size = fstream.tellg();
			
			if((uint64_t)file_size > (uint64_t)max_size) // ensure a large domain for comparison, and negative -> too large
				return false;//don't go crazy
			size_t file_size_t = static_cast<size_t>(file_size);

			target_str.resize(file_size_t);

			fstream.seekg (0, std::ios::beg);
			fstream.read((char*)target_str.data(), target_str.size());
			fstream.close();
			return true;
		}

		catch(...)
		{
			return false;
		}
#endif
	}
}
}
