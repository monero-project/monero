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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <unistd.h>
#include <cstdio>
#include <wchar.h>

#ifdef __GLIBC__
#include <gnu/libc-version.h>
#endif

#ifdef __GLIBC__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <string>
#endif

//tools::is_hdd
#ifdef __GLIBC__
  #include <sstream>
  #include <sys/sysmacros.h>
  #include <fstream>
#endif

#include "unbound.h"

#include "include_base_utils.h"
#include "file_io_utils.h"
#include "wipeable_string.h"
#include "time_helper.h"
using namespace epee;

#include "crypto/crypto.h"
#include "util.h"
#include "stack_trace.h"
#include "memwipe.h"
#include "net/http_client.h"                        // epee::net_utils::...
#include "readline_buffer.h"

#ifdef WIN32
#ifndef STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_DEPRECATE
#endif
  #include <windows.h>
  #include <shlobj.h>
  #include <strsafe.h>
#else 
  #include <sys/file.h>
  #include <sys/utsname.h>
  #include <sys/stat.h>
#endif
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <openssl/evp.h>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "util"

namespace
{

#ifndef _WIN32
static int flock_exnb(int fd)
{
  struct flock fl;
  int ret;

  memset(&fl, 0, sizeof(fl));
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;
  ret = fcntl(fd, F_SETLK, &fl);
  if (ret < 0)
    MERROR("Error locking fd " << fd << ": " << errno << " (" << strerror(errno) << ")");
  return ret;
}
#endif

}

namespace tools
{
  std::function<void(int)> signal_handler::m_handler;

  private_file::private_file() noexcept : m_handle(), m_filename() {}

  private_file::private_file(std::FILE* handle, std::string&& filename) noexcept
    : m_handle(handle), m_filename(std::move(filename)) {}

  private_file private_file::create(std::string name)
  {
#ifdef WIN32
    struct close_handle
    {
      void operator()(HANDLE handle) const noexcept
      {
        CloseHandle(handle);
      }
    };

    std::unique_ptr<void, close_handle> process = nullptr;
    {
      HANDLE temp{};
      const bool fail = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, std::addressof(temp)) == 0;
      process.reset(temp);
      if (fail)
        return {};
    }

    DWORD sid_size = 0;
    GetTokenInformation(process.get(), TokenOwner, nullptr, 0, std::addressof(sid_size));
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      return {};

    std::unique_ptr<char[]> sid{new char[sid_size]};
    if (!GetTokenInformation(process.get(), TokenOwner, sid.get(), sid_size, std::addressof(sid_size)))
      return {};

    const PSID psid = reinterpret_cast<const PTOKEN_OWNER>(sid.get())->Owner;
    const DWORD daclSize =
      sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD);

    const std::unique_ptr<char[]> dacl{new char[daclSize]};
    if (!InitializeAcl(reinterpret_cast<PACL>(dacl.get()), daclSize, ACL_REVISION))
      return {};

    if (!AddAccessAllowedAce(reinterpret_cast<PACL>(dacl.get()), ACL_REVISION, (READ_CONTROL | FILE_GENERIC_READ | DELETE), psid))
      return {};

    SECURITY_DESCRIPTOR descriptor{};
    if (!InitializeSecurityDescriptor(std::addressof(descriptor), SECURITY_DESCRIPTOR_REVISION))
      return {};

    if (!SetSecurityDescriptorDacl(std::addressof(descriptor), true, reinterpret_cast<PACL>(dacl.get()), false))
      return {};

    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), std::addressof(descriptor), false};
    std::unique_ptr<void, close_handle> file{
      CreateFile(
        name.c_str(),
        GENERIC_WRITE, FILE_SHARE_READ,
        std::addressof(attributes),
        CREATE_NEW, (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE),
        nullptr
      )
    };
    if (file)
    {
      const int fd = _open_osfhandle(reinterpret_cast<intptr_t>(file.get()), 0);
      if (0 <= fd)
      {
        file.release();
        std::FILE* real_file = _fdopen(fd, "w");
        if (!real_file)
        {
          _close(fd);
        }
        return {real_file, std::move(name)};
      }
    }
#else
    const int fdr = open(name.c_str(), (O_RDONLY | O_CREAT), S_IRUSR);
    if (0 <= fdr)
    {
      struct stat rstats = {};
      if (fstat(fdr, std::addressof(rstats)) != 0)
      {
        close(fdr);
        return {};
      }
      fchmod(fdr, (S_IRUSR | S_IWUSR));
      const int fdw = open(name.c_str(), O_RDWR);
      fchmod(fdr, rstats.st_mode);
      close(fdr);

      if (0 <= fdw)
      {
        struct stat wstats = {};
        if (fstat(fdw, std::addressof(wstats)) == 0 &&
            rstats.st_dev == wstats.st_dev && rstats.st_ino == wstats.st_ino &&
            flock_exnb(fdw) == 0 && ftruncate(fdw, 0) == 0)
        {
          std::FILE* file = fdopen(fdw, "w");
          if (file) return {file, std::move(name)};
        }
        close(fdw);
      }
    }
#endif
    return {};
  }

  private_file::~private_file() noexcept
  {
    try
    {
      boost::system::error_code ec{};
      boost::filesystem::remove(filename(), ec);
    }
    catch (...) {}
  }

  file_locker::file_locker(const std::string &filename)
  {
#ifdef WIN32
    m_fd = INVALID_HANDLE_VALUE;
    std::wstring filename_wide;
    try
    {
      filename_wide = string_tools::utf8_to_utf16(filename);
    }
    catch (const std::exception &e)
    {
      MERROR("Failed to convert path \"" << filename << "\" to UTF-16: " << e.what());
      return;
    }
    m_fd = CreateFileW(filename_wide.c_str(), GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_fd != INVALID_HANDLE_VALUE)
    {
      OVERLAPPED ov;
      memset(&ov, 0, sizeof(ov));
      if (!LockFileEx(m_fd, LOCKFILE_FAIL_IMMEDIATELY | LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &ov))
      {
        MERROR("Failed to lock " << filename << ": " << std::error_code(GetLastError(), std::system_category()));
        CloseHandle(m_fd);
        m_fd = INVALID_HANDLE_VALUE;
      }
    }
    else
    {
      MERROR("Failed to open " << filename << ": " << std::error_code(GetLastError(), std::system_category()));
    }
#else
    m_fd = open(filename.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0666);
    if (m_fd != -1)
    {
      if (flock_exnb(m_fd) == -1)
      {
        MERROR("Failed to lock " << filename << ": " << std::strerror(errno));
        close(m_fd);
        m_fd = -1;
      }
    }
    else
    {
      MERROR("Failed to open " << filename << ": " << std::strerror(errno));
    }
#endif
  }
  file_locker::~file_locker()
  {
    if (locked())
    {
#ifdef WIN32
      CloseHandle(m_fd);
#else
      close(m_fd);
#endif
    }
  }
  bool file_locker::locked() const
  {
#ifdef WIN32
    return m_fd != INVALID_HANDLE_VALUE;
#else
    return m_fd != -1;
#endif
  }

#ifdef WIN32
  std::string get_windows_version_display_string()
  {
    typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
    typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);
#define BUFSIZE 10000

    char pszOS[BUFSIZE] = {0};
    OSVERSIONINFOEX osvi;
    SYSTEM_INFO si;
    PGNSI pGNSI;
    PGPI pGPI;
    BOOL bOsVersionInfoEx;
    DWORD dwType;

    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi);

    if(!bOsVersionInfoEx) return pszOS;

    // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

    pGNSI = (PGNSI) GetProcAddress(
      GetModuleHandle(TEXT("kernel32.dll")), 
      "GetNativeSystemInfo");
    if(NULL != pGNSI)
      pGNSI(&si);
    else GetSystemInfo(&si);

    if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && 
      osvi.dwMajorVersion > 4 )
    {
      StringCchCopy(pszOS, BUFSIZE, TEXT("Microsoft "));

      // Test for the specific product.
      if ( osvi.dwMajorVersion == 10 )
      {
        if ( osvi.dwMinorVersion == 0 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            StringCchCat(pszOS, BUFSIZE, TEXT("Windows 10 "));
          else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2016 " ));
        }
      }

      if ( osvi.dwMajorVersion == 6 )
      {
        if ( osvi.dwMinorVersion == 0 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            StringCchCat(pszOS, BUFSIZE, TEXT("Windows Vista "));
          else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2008 " ));
        }

        if ( osvi.dwMinorVersion == 1 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            StringCchCat(pszOS, BUFSIZE, TEXT("Windows 7 "));
          else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2008 R2 " ));
        }

        if ( osvi.dwMinorVersion == 2 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            StringCchCat(pszOS, BUFSIZE, TEXT("Windows 8 "));
          else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2012 " ));
        }

        if ( osvi.dwMinorVersion == 3 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            StringCchCat(pszOS, BUFSIZE, TEXT("Windows 8.1 "));
          else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2012 R2 " ));
        }

        pGPI = (PGPI) GetProcAddress(
          GetModuleHandle(TEXT("kernel32.dll")), 
          "GetProductInfo");

        pGPI( osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

        switch( dwType )
        {
        case PRODUCT_ULTIMATE:
          StringCchCat(pszOS, BUFSIZE, TEXT("Ultimate Edition" ));
          break;
        case PRODUCT_PROFESSIONAL:
          StringCchCat(pszOS, BUFSIZE, TEXT("Professional" ));
          break;
        case PRODUCT_HOME_PREMIUM:
          StringCchCat(pszOS, BUFSIZE, TEXT("Home Premium Edition" ));
          break;
        case PRODUCT_HOME_BASIC:
          StringCchCat(pszOS, BUFSIZE, TEXT("Home Basic Edition" ));
          break;
        case PRODUCT_ENTERPRISE:
          StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition" ));
          break;
        case PRODUCT_BUSINESS:
          StringCchCat(pszOS, BUFSIZE, TEXT("Business Edition" ));
          break;
        case PRODUCT_STARTER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Starter Edition" ));
          break;
        case PRODUCT_CLUSTER_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Cluster Server Edition" ));
          break;
        case PRODUCT_DATACENTER_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Datacenter Edition" ));
          break;
        case PRODUCT_DATACENTER_SERVER_CORE:
          StringCchCat(pszOS, BUFSIZE, TEXT("Datacenter Edition (core installation)" ));
          break;
        case PRODUCT_ENTERPRISE_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition" ));
          break;
        case PRODUCT_ENTERPRISE_SERVER_CORE:
          StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition (core installation)" ));
          break;
        case PRODUCT_ENTERPRISE_SERVER_IA64:
          StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition for Itanium-based Systems" ));
          break;
        case PRODUCT_SMALLBUSINESS_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Small Business Server" ));
          break;
        case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
          StringCchCat(pszOS, BUFSIZE, TEXT("Small Business Server Premium Edition" ));
          break;
        case PRODUCT_STANDARD_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Standard Edition" ));
          break;
        case PRODUCT_STANDARD_SERVER_CORE:
          StringCchCat(pszOS, BUFSIZE, TEXT("Standard Edition (core installation)" ));
          break;
        case PRODUCT_WEB_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Web Server Edition" ));
          break;
        }
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
      {
        if( GetSystemMetrics(SM_SERVERR2) )
          StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Server 2003 R2, "));
        else if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER )
          StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Storage Server 2003"));
        else if ( osvi.wSuiteMask & VER_SUITE_WH_SERVER )
          StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Home Server"));
        else if( osvi.wProductType == VER_NT_WORKSTATION &&
          si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
        {
          StringCchCat(pszOS, BUFSIZE, TEXT( "Windows XP Professional x64 Edition"));
        }
        else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2003, "));

        // Test for the server type.
        if ( osvi.wProductType != VER_NT_WORKSTATION )
        {
          if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
          {
            if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Edition for Itanium-based Systems" ));
            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise Edition for Itanium-based Systems" ));
          }

          else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
          {
            if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter x64 Edition" ));
            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise x64 Edition" ));
            else StringCchCat(pszOS, BUFSIZE, TEXT( "Standard x64 Edition" ));
          }

          else
          {
            if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Compute Cluster Edition" ));
            else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Edition" ));
            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise Edition" ));
            else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Web Edition" ));
            else StringCchCat(pszOS, BUFSIZE, TEXT( "Standard Edition" ));
          }
        }
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
      {
        StringCchCat(pszOS, BUFSIZE, TEXT("Windows XP "));
        if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
          StringCchCat(pszOS, BUFSIZE, TEXT( "Home Edition" ));
        else StringCchCat(pszOS, BUFSIZE, TEXT( "Professional" ));
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
      {
        StringCchCat(pszOS, BUFSIZE, TEXT("Windows 2000 "));

        if ( osvi.wProductType == VER_NT_WORKSTATION )
        {
          StringCchCat(pszOS, BUFSIZE, TEXT( "Professional" ));
        }
        else 
        {
          if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
            StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Server" ));
          else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
            StringCchCat(pszOS, BUFSIZE, TEXT( "Advanced Server" ));
          else StringCchCat(pszOS, BUFSIZE, TEXT( "Server" ));
        }
      }

      // Include service pack (if any) and build number.

      if( strlen(osvi.szCSDVersion) > 0 )
      {
        StringCchCat(pszOS, BUFSIZE, TEXT(" ") );
        StringCchCat(pszOS, BUFSIZE, osvi.szCSDVersion);
      }

      TCHAR buf[80];

      StringCchPrintf( buf, 80, TEXT(" (build %d)"), osvi.dwBuildNumber);
      StringCchCat(pszOS, BUFSIZE, buf);

      if ( osvi.dwMajorVersion >= 6 )
      {
        if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
          StringCchCat(pszOS, BUFSIZE, TEXT( ", 64-bit" ));
        else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
          StringCchCat(pszOS, BUFSIZE, TEXT(", 32-bit"));
      }

      return pszOS; 
    }
    else
    {  
      printf( "This sample does not support this version of Windows.\n");
      return pszOS;
    }
  }
#else
std::string get_nix_version_display_string()
{
  struct utsname un;

  if(uname(&un) < 0)
    return std::string("*nix: failed to get os version");
  return std::string() + un.sysname + " " + un.version + " " + un.release;
}
#endif



  std::string get_os_version_string()
  {
#ifdef WIN32
    return get_windows_version_display_string();
#else
    return get_nix_version_display_string();
#endif
  }



#ifdef WIN32
  std::string get_special_folder_path(int nfolder, bool iscreate)
  {
    WCHAR psz_path[MAX_PATH] = L"";

    if (SHGetSpecialFolderPathW(NULL, psz_path, nfolder, iscreate))
    {
      try
      {
        return string_tools::utf16_to_utf8(psz_path);
      }
      catch (const std::exception &e)
      {
        MERROR("utf16_to_utf8 failed: " << e.what());
        return "";
      }
    }

    LOG_ERROR("SHGetSpecialFolderPathW() failed, could not obtain requested path.");
    return "";
  }
#endif
  
  std::string get_default_data_dir()
  {
    /* Please for the love of god refactor  the ifdefs out of this */

    // namespace fs = boost::filesystem;
    // Windows < Vista: C:\Documents and Settings\Username\Application Data\CRYPTONOTE_NAME
    // Windows >= Vista: C:\Users\Username\AppData\Roaming\CRYPTONOTE_NAME
    // Unix & Mac: ~/.CRYPTONOTE_NAME
    std::string config_folder;

#ifdef WIN32
    config_folder = get_special_folder_path(CSIDL_COMMON_APPDATA, true) + "\\" + CRYPTONOTE_NAME;
#else
    std::string pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
      pathRet = "/";
    else
      pathRet = pszHome;
    config_folder = (pathRet + "/." + CRYPTONOTE_NAME);
#endif

    return config_folder;
  }

  bool create_directories_if_necessary(const std::string& path)
  {
    namespace fs = boost::filesystem;
    boost::system::error_code ec;
    fs::path fs_path(path);
    if (fs::is_directory(fs_path, ec))
    {
      return true;
    }

    bool res = fs::create_directories(fs_path, ec);
    if (res)
    {
      LOG_PRINT_L2("Created directory: " << path);
    }
    else
    {
      LOG_PRINT_L2("Can't create directory: " << path << ", err: "<< ec.message());
    }

    return res;
  }

  std::error_code replace_file(const std::string& old_name, const std::string& new_name)
  {
    int code;
#if defined(WIN32)
    // Maximizing chances for success
    std::wstring wide_replacement_name;
    try { wide_replacement_name = string_tools::utf8_to_utf16(old_name); }
    catch (...) { return std::error_code(GetLastError(), std::system_category()); }
    std::wstring wide_replaced_name;
    try { wide_replaced_name = string_tools::utf8_to_utf16(new_name); }
    catch (...) { return std::error_code(GetLastError(), std::system_category()); }

    DWORD attributes = ::GetFileAttributesW(wide_replaced_name.c_str());
    if (INVALID_FILE_ATTRIBUTES != attributes)
    {
      ::SetFileAttributesW(wide_replaced_name.c_str(), attributes & (~FILE_ATTRIBUTE_READONLY));
    }

    bool ok = 0 != ::MoveFileExW(wide_replacement_name.c_str(), wide_replaced_name.c_str(), MOVEFILE_REPLACE_EXISTING);
    code = ok ? 0 : static_cast<int>(::GetLastError());
#else
    bool ok = 0 == std::rename(old_name.c_str(), new_name.c_str());
    code = ok ? 0 : errno;
#endif
    return std::error_code(code, std::system_category());
  }

  static bool unbound_built_with_threads()
  {
    ub_ctx *ctx = ub_ctx_create();
    if (!ctx) return false; // cheat a bit, should not happen unless OOM
    char *monero = strdup("monero"), *unbound = strdup("unbound");
    ub_ctx_zone_add(ctx, monero, unbound); // this calls ub_ctx_finalize first, then errors out with UB_SYNTAX
    free(unbound);
    free(monero);
    // if no threads, bails out early with UB_NOERROR, otherwise fails with UB_AFTERFINAL id already finalized
    bool with_threads = ub_ctx_async(ctx, 1) != 0; // UB_AFTERFINAL is not defined in public headers, check any error
    ub_ctx_delete(ctx);
    MINFO("libunbound was built " << (with_threads ? "with" : "without") << " threads");
    return with_threads;
  }

  bool sanitize_locale()
  {
    // boost::filesystem throws for "invalid" locales, such as en_US.UTF-8, or kjsdkfs,
    // so reset it here before any calls to it
    try
    {
      boost::filesystem::path p {std::string("test")};
      p /= std::string("test");
    }
    catch (...)
    {
#if defined(__MINGW32__) || defined(__MINGW__)
      putenv("LC_ALL=C");
      putenv("LANG=C");
#else
      setenv("LC_ALL", "C", 1);
      setenv("LANG", "C", 1);
#endif
      return true;
    }
    return false;
  }

#ifdef STACK_TRACE
#ifdef _WIN32
  // https://stackoverflow.com/questions/1992816/how-to-handle-seg-faults-under-windows
  static LONG WINAPI windows_crash_handler(PEXCEPTION_POINTERS pExceptionInfo)
  {
    tools::log_stack_trace("crashing");
    exit(1);
    return EXCEPTION_CONTINUE_SEARCH;
  }
  static void setup_crash_dump()
  {
    SetUnhandledExceptionFilter(windows_crash_handler);
  }
#else
  static void posix_crash_handler(int signal)
  {
    tools::log_stack_trace(("crashing with fatal signal " + std::to_string(signal)).c_str());
#ifdef NDEBUG
    _exit(1);
#else
    abort();
#endif
  }
  static void setup_crash_dump()
  {
    signal(SIGSEGV, posix_crash_handler);
    signal(SIGBUS, posix_crash_handler);
    signal(SIGILL, posix_crash_handler);
    signal(SIGFPE, posix_crash_handler);
  }
#endif
#else
  static void setup_crash_dump() {}
#endif

  bool disable_core_dumps()
  {
#ifdef __GLIBC__
    // disable core dumps in release mode
    struct rlimit rlimit;
    rlimit.rlim_cur = rlimit.rlim_max = 0;
    if (setrlimit(RLIMIT_CORE, &rlimit))
    {
      MWARNING("Failed to disable core dumps");
      return false;
    }
#endif
    return true;
  }

  ssize_t get_lockable_memory()
  {
#ifdef __GLIBC__
    struct rlimit rlim;
    if (getrlimit(RLIMIT_MEMLOCK, &rlim) < 0)
    {
      MERROR("Failed to determine the lockable memory limit");
      return -1;
    }
    return rlim.rlim_cur;
#else
    return -1;
#endif
  }

  bool on_startup()
  {
    mlog_configure("", true);

    setup_crash_dump();

    sanitize_locale();

#ifdef __GLIBC__
    const char *ver = gnu_get_libc_version();
    if (!strcmp(ver, "2.25"))
      MCLOG_RED(el::Level::Warning, "global", "Running with glibc " << ver << ", hangs may occur - change glibc version if possible");
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000 || defined(LIBRESSL_VERSION_TEXT)
    SSL_library_init();
#else
    OPENSSL_init_ssl(0, NULL);
#endif

    if (!unbound_built_with_threads())
      MCLOG_RED(el::Level::Warning, "global", "libunbound was not built with threads enabled - crashes may occur");

    return true;
  }
  void set_strict_default_file_permissions(bool strict)
  {
#if defined(__MINGW32__) || defined(__MINGW__)
    // no clue about the odd one out
#else
    mode_t mode = strict ? 077 : 0;
    umask(mode);
#endif
  }

  boost::optional<bool> is_hdd(const char *file_path)
  {
#ifdef __GLIBC__
    struct stat st;
    std::string prefix;
    if(stat(file_path, &st) == 0)
    {
      std::ostringstream s;
      s << "/sys/dev/block/" << major(st.st_dev) << ":" << minor(st.st_dev);
      prefix = s.str();
    }
    else
    {
      return boost::none;
    }
    std::string attr_path = prefix + "/queue/rotational";
    std::ifstream f(attr_path, std::ios_base::in);
    if(not f.is_open())
    {
      attr_path = prefix + "/../queue/rotational";
      f.open(attr_path, std::ios_base::in);
      if(not f.is_open())
      {
          return boost::none;
      }
    }
    unsigned short val = 0xdead;
    f >> val;
    if(not f.fail())
    {
      return (val == 1);
    }
    return boost::none;
#else
    return boost::none;
#endif
  }

  namespace
  {
    boost::mutex max_concurrency_lock;
    unsigned max_concurrency = boost::thread::hardware_concurrency();
  }

  void set_max_concurrency(unsigned n)
  {
    if (n < 1)
      n = boost::thread::hardware_concurrency();
    unsigned hwc = boost::thread::hardware_concurrency();
    if (n > hwc)
      n = hwc;
    boost::lock_guard<boost::mutex> lock(max_concurrency_lock);
    max_concurrency = n;
  }

  unsigned get_max_concurrency()
  {
    boost::lock_guard<boost::mutex> lock(max_concurrency_lock);
    return max_concurrency;
  }

  bool is_privacy_preserving_network(const std::string &address)
  {
    if (boost::ends_with(address, ".onion"))
      return true;
    if (boost::ends_with(address, ".i2p"))
      return true;
    return false;
  }

  bool is_local_address(const std::string &address)
  {
    // always assume Tor/I2P addresses to be untrusted by default
    if (is_privacy_preserving_network(address))
    {
      MDEBUG("Address '" << address << "' is Tor/I2P, non local");
      return false;
    }

    // extract host
    epee::net_utils::http::url_content u_c;
    if (!epee::net_utils::parse_url(address, u_c))
    {
      MWARNING("Failed to determine whether address '" << address << "' is local, assuming not");
      return false;
    }
    if (u_c.host.empty())
    {
      MWARNING("Failed to determine whether address '" << address << "' is local, assuming not");
      return false;
    }

    // resolve to IP
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(u_c.host, "");
    boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query);
    while (i != boost::asio::ip::tcp::resolver::iterator())
    {
      const boost::asio::ip::tcp::endpoint &ep = *i;
      if (ep.address().is_loopback())
      {
        MDEBUG("Address '" << address << "' is local");
        return true;
      }
      ++i;
    }

    MDEBUG("Address '" << address << "' is not local");
    return false;
  }
  int vercmp(const char *v0, const char *v1)
  {
    std::vector<std::string> f0, f1;
    boost::split(f0, v0, boost::is_any_of(".-"));
    boost::split(f1, v1, boost::is_any_of(".-"));
    for (size_t i = 0; i < std::max(f0.size(), f1.size()); ++i) {
      if (i >= f0.size())
        return -1;
      if (i >= f1.size())
        return 1;
      int f0i = atoi(f0[i].c_str()), f1i = atoi(f1[i].c_str());
      int n = f0i - f1i;
      if (n)
        return n;
    }
    return 0;
  }

  bool sha256sum(const uint8_t *data, size_t len, crypto::hash &hash)
  {
    return EVP_Digest(data, len, (unsigned char*) hash.data, NULL, EVP_sha256(), NULL) != 0;
  }

  bool sha256sum(const std::string &filename, crypto::hash &hash)
  {
    if (!epee::file_io_utils::is_file_exist(filename))
      return false;
    std::ifstream f;
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    f.open(filename, std::ios_base::binary | std::ios_base::in | std::ios::ate);
    if (!f)
      return false;
    std::ifstream::pos_type file_size = f.tellg();
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr))
      return false;
    size_t size_left = file_size;
    f.seekg(0, std::ios::beg);
    while (size_left)
    {
      char buf[4096];
      std::ifstream::pos_type read_size = size_left > sizeof(buf) ? sizeof(buf) : size_left;
      f.read(buf, read_size);
      if (!f || !f.good())
        return false;
      if (!EVP_DigestUpdate(ctx.get(), buf, read_size))
        return false;
      size_left -= read_size;
    }
    f.close();
    if (!EVP_DigestFinal_ex(ctx.get(), (unsigned char*)hash.data, nullptr))
      return false;
    return true;
  }

  boost::optional<std::pair<uint32_t, uint32_t>> parse_subaddress_lookahead(const std::string& str)
  {
    auto pos = str.find(":");
    bool r = pos != std::string::npos;
    uint32_t major;
    r = r && epee::string_tools::get_xtype_from_string(major, str.substr(0, pos));
    uint32_t minor;
    r = r && epee::string_tools::get_xtype_from_string(minor, str.substr(pos + 1));
    if (r)
    {
      return std::make_pair(major, minor);
    }
    else
    {
      return {};
    }
  }

  std::string glob_to_regex(const std::string &val)
  {
    std::string newval;

    bool escape = false;
    for (char c: val)
    {
      if (c == '*')
        newval += escape ? "*" : ".*", escape = false;
      else if (c == '?')
        newval += escape ? "?" : ".", escape = false;
      else if (c == '\\')
        newval += '\\', escape = !escape;
      else
        newval += c, escape = false;
    }
    return newval;
  }
  
#ifdef _WIN32
  std::string input_line_win()
  {
    HANDLE hConIn = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    DWORD oldMode;

    FlushConsoleInputBuffer(hConIn);
    GetConsoleMode(hConIn, &oldMode);
    SetConsoleMode(hConIn, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);

    wchar_t buffer[1024];
    DWORD read;

    ReadConsoleW(hConIn, buffer, sizeof(buffer)/sizeof(wchar_t)-1, &read, nullptr);
    buffer[read] = 0;

    SetConsoleMode(hConIn, oldMode);
    CloseHandle(hConIn);
  
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, NULL, 0, NULL, NULL);
    std::string buf(size_needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, buffer, -1, &buf[0], size_needed, NULL, NULL);
    buf.pop_back(); //size_needed includes null that we needed to have space for
    return buf;
  }
#endif

  void closefrom(int fd)
  {
#if defined __FreeBSD__ || defined __OpenBSD__ || defined __NetBSD__ || defined __DragonFly__
    ::closefrom(fd);
#else
#if defined __GLIBC__
    const int sc_open_max =  sysconf(_SC_OPEN_MAX);
    const int MAX_FDS = std::min(65536, sc_open_max);
#else
    const int MAX_FDS = 65536;
#endif
    while (fd < MAX_FDS)
    {
      close(fd);
      ++fd;
    }
#endif
  }

  std::string get_human_readable_timestamp(uint64_t ts)
  {
    char buffer[64];
    if (ts < 1234567890)
      return "<unknown>";
    time_t tt = ts;
    struct tm tm;
    misc_utils::get_gmt_time(tt, tm);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer);
  }

  std::string get_human_readable_timespan(uint64_t seconds)
  {
    if (seconds < 60)
      return std::to_string(seconds) + " seconds";
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);
    if (seconds < 3600)
    {
      ss << seconds / 60.f;
      return ss.str() + " minutes";
    }
    if (seconds < 3600 * 24)
    {
      ss << seconds / 3600.f;
      return ss.str() + " hours";
    }
    if (seconds < 3600 * 24 * 30.5f)
    {
      ss << seconds / (3600 * 24.f);
      return ss.str() + " days";
    }
    if (seconds < 3600 * 24 * 365.25f)
    {
      ss << seconds / (3600 * 24 * 30.5f);
      return ss.str() + " months";
    }
    if (seconds < 3600 * 24 * 365.25f * 100)
    {
      ss << seconds / (3600 * 24 * 365.25f);
      return ss.str() + " years";
    }
    return "a long time";
  }

  std::string get_human_readable_bytes(uint64_t bytes)
  {
    // Use 1024 for "kilo", 1024*1024 for "mega" and so on instead of the more modern and standard-conforming
    // 1000, 1000*1000 and so on, to be consistent with other Monero code that also uses base 2 units
    struct byte_map
    {
        const char* const format;
        const std::uint64_t bytes;
    };

    static constexpr const byte_map sizes[] =
    {
        {"%.0f B", 1024},
        {"%.2f kB", 1024 * 1024},
        {"%.2f MB", std::uint64_t(1024) * 1024 * 1024},
        {"%.2f GB", std::uint64_t(1024) * 1024 * 1024 * 1024},
        {"%.2f TB", std::uint64_t(1024) * 1024 * 1024 * 1024 * 1024}
    };

    struct bytes_less
    {
        bool operator()(const byte_map& lhs, const byte_map& rhs) const noexcept
        {
            return lhs.bytes < rhs.bytes;
        }
    };

    const auto size = std::upper_bound(
        std::begin(sizes), std::end(sizes) - 1, byte_map{"", bytes}, bytes_less{}
    );
    const std::uint64_t divisor = size->bytes / 1024;
    return (boost::format(size->format) % (double(bytes) / divisor)).str();
  }

  void clear_screen()
  {
    std::cout << "\033[2K" << std::flush; // clear whole line
    std::cout << "\033c" << std::flush; // clear current screen and scrollback
    std::cout << "\033[2J" << std::flush; // clear current screen only, scrollback is still around
    std::cout << "\033[3J" << std::flush; // does nothing, should clear current screen and scrollback
    std::cout << "\033[1;1H" << std::flush; // move cursor top/left
    std::cout << "\r                                                \r" << std::flush; // erase odd chars if the ANSI codes were printed raw
#ifdef _WIN32
    COORD coord{0, 0};
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(h, &csbi))
    {
      DWORD cbConSize = csbi.dwSize.X * csbi.dwSize.Y, w;
      FillConsoleOutputCharacter(h, (TCHAR)' ', cbConSize, coord, &w);
      if (GetConsoleScreenBufferInfo(h, &csbi))
        FillConsoleOutputAttribute(h, csbi.wAttributes, cbConSize, coord, &w);
      SetConsoleCursorPosition(h, coord);
    }
#endif
  }

  std::pair<std::string, size_t> get_string_prefix_by_width(const std::string &s, size_t columns)
  {
    std::string sc = "";
    size_t avail = s.size();
    const char *ptr = s.data();
    wint_t cp = 0;
    int bytes = 1;
    size_t sw = 0;
    char wbuf[8], *wptr;
    while (avail--)
    {
      if ((*ptr & 0x80) == 0)
      {
        cp = *ptr++;
        bytes = 1;
      }
      else if ((*ptr & 0xe0) == 0xc0)
      {
        if (avail < 1)
        {
          MERROR("Invalid UTF-8");
          return std::make_pair(s, s.size());
        }
        cp = (*ptr++ & 0x1f) << 6;
        cp |= *ptr++ & 0x3f;
        --avail;
        bytes = 2;
      }
      else if ((*ptr & 0xf0) == 0xe0)
      {
        if (avail < 2)
        {
          MERROR("Invalid UTF-8");
          return std::make_pair(s, s.size());
        }
        cp = (*ptr++ & 0xf) << 12;
        cp |= (*ptr++ & 0x3f) << 6;
        cp |= *ptr++ & 0x3f;
        avail -= 2;
        bytes = 3;
      }
      else if ((*ptr & 0xf8) == 0xf0)
      {
        if (avail < 3)
        {
          MERROR("Invalid UTF-8");
          return std::make_pair(s, s.size());
        }
        cp = (*ptr++ & 0x7) << 18;
        cp |= (*ptr++ & 0x3f) << 12;
        cp |= (*ptr++ & 0x3f) << 6;
        cp |= *ptr++ & 0x3f;
        avail -= 3;
        bytes = 4;
      }
      else
      {
        MERROR("Invalid UTF-8");
        return std::make_pair(s, s.size());
      }

      wptr = wbuf;
      switch (bytes)
      {
        case 1: *wptr++ = cp; break;
        case 2: *wptr++ = 0xc0 | (cp >> 6); *wptr++ = 0x80 | (cp & 0x3f); break;
        case 3: *wptr++ = 0xe0 | (cp >> 12); *wptr++ = 0x80 | ((cp >> 6) & 0x3f); *wptr++ = 0x80 | (cp & 0x3f); break;
        case 4: *wptr++ = 0xf0 | (cp >> 18); *wptr++ = 0x80 | ((cp >> 12) & 0x3f); *wptr++ = 0x80 | ((cp >> 6) & 0x3f); *wptr++ = 0x80 | (cp & 0x3f); break;
        default: MERROR("Invalid UTF-8"); return std::make_pair(s, s.size());
      }
      *wptr = 0;
      sc += std::string(wbuf, bytes);
#ifdef _WIN32
      int cpw = 1; // Guess who does not implement wcwidth
#else
      int cpw = wcwidth(cp);
#endif
      if (cpw > 0)
      {
        if (cpw > (int)columns)
          break;
        columns -= cpw;
        sw += cpw;
      }
      cp = 0;
      bytes = 1;
    }
    return std::make_pair(sc, sw);
  }

  size_t get_string_width(const std::string &s)
  {
    return get_string_prefix_by_width(s, 999999999).second;
  };

  std::vector<std::pair<std::string, size_t>> split_line_by_width(const std::string &s, size_t columns)
  {
    std::vector<std::string> words;
    std::vector<std::pair<std::string, size_t>> lines;
    boost::split(words, s, boost::is_any_of(" "), boost::token_compress_on);
    // split large "words"
    for (size_t i = 0; i < words.size(); ++i)
    {
      for (;;)
      {
        std::string prefix = get_string_prefix_by_width(words[i], columns).first;
        if (prefix == words[i])
          break;
        words[i] = words[i].substr(prefix.size());
        words.insert(words.begin() + i, prefix);
      }
    }

    lines.push_back(std::make_pair("", 0));
    while (!words.empty())
    {
      const size_t word_len = get_string_width(words.front());
      size_t line_len = get_string_width(lines.back().first);
      if (line_len > 0 && line_len + 1 + word_len > columns)
      {
        lines.push_back(std::make_pair("", 0));
        line_len = 0;
      }
      if (line_len > 0)
      {
        lines.back().first += " ";
        lines.back().second++;
      }
      lines.back().first += words.front();
      lines.back().second += word_len;
      words.erase(words.begin());
    }
    return lines;
  }

  // Calculate a "sync weight" over ranges of blocks in the blockchain, suitable for
  // calculating sync time estimates
  uint64_t cumulative_block_sync_weight(cryptonote::network_type nettype, uint64_t start_block, uint64_t num_blocks)
  {
    if (nettype != cryptonote::MAINNET)
    {
      // No detailed data available except for Mainnet: Give back the number of blocks
      // as a very simple and non-varying block sync weight for ranges of Testnet and
      // Stagenet blocks
      return num_blocks;
    }

    // The following is a table of average blocks sizes in bytes over the Monero mainnet
    // blockchain, where the block size is averaged over ranges of 10,000 blocks
    // (about 2 weeks worth of blocks each).
    // The first array entry of 442 thus means "The average byte size of the blocks
    // 0 .. 9,999 is 442". The info "block_size" from the "get_block_header_by_height"
    // RPC call was used for calculating this. This table (and the whole mechanism
    // of calculating a "sync weight") is most important when estimating times for
    // syncing from scratch. Without it the fast progress through the (in comparison)
    // rather small blocks in the early blockchain) would lead to vastly underestimated
    // total sync times.
    // It's no big problem for estimates that this table will, over time, and if not
    // updated, miss larger and larger parts at the top of the blockchain, as long
    // as block size averages there do not differ wildly.
    // Without time-consuming tests it's hard to say how much the estimates would
    // improve if one would not only take block sizes into account, but also varying
    // verification times i.e. the different CPU effort needed for the different
    // transaction types (pre / post RingCT, pre / post Bulletproofs).
    // Testnet and Stagenet are neglected here because of their much smaller
    // importance.
    static const uint32_t average_block_sizes[] =
    {
      442, 1211, 1445, 1763, 2272, 8217, 5603, 9999, 16358, 10805, 5290, 4362,
      4325, 5584, 4515, 5008, 4789, 5196, 7660, 3829, 6034, 2925, 3762, 2545,
      2437, 2553, 2167, 2761, 2015, 1969, 2350, 1731, 2367, 2078, 2026, 3518,
      2214, 1908, 1780, 1640, 1976, 1647, 1921, 1716, 1895, 2150, 2419, 2451,
      2147, 2327, 2251, 1644, 1750, 1481, 1570, 1524, 1562, 1668, 1386, 1494,
      1637, 1880, 1431, 1472, 1637, 1363, 1762, 1597, 1999, 1564, 1341, 1388,
      1530, 1476, 1617, 1488, 1368, 1906, 1403, 1695, 1535, 1598, 1318, 1234,
      1358, 1406, 1698, 1554, 1591, 1758, 1426, 2389, 1946, 1533, 1308, 2701,
      1525, 1653, 3580, 1889, 2913, 8164, 5154, 3762, 3356, 4360, 3589, 4844,
      4232, 3781, 3882, 5924, 10790, 7185, 7442, 8214, 8509, 7484, 6939, 7391,
      8210, 15572, 39680, 44810, 53873, 54639, 68227, 63428, 62386, 68504,
      83073, 103858, 117573, 98089, 96793, 102337, 94714, 129568, 251584,
      132026, 94579, 94516, 95722, 106495, 121824, 153983, 162338, 136608,
      137104, 109872, 91114, 84757, 96339, 74251, 94314, 143216, 155837,
      129968, 120201, 109913, 101588, 97332, 104611, 95310, 93419, 113345,
      100743, 92152, 57565, 22533, 37564, 21823, 19980, 18277, 18402, 14344,
      12142, 15842, 13677, 17631, 18294, 22270, 41422, 39296, 36688, 33512,
      33831, 27582, 22276, 27516, 27317, 25505, 24426, 20566, 23045, 26766,
      28185, 26169, 27011, 28642, 34994, 34442, 30682, 34357, 31640, 41167,
      41301, 48616, 51075, 55061, 49909, 44606, 47091, 53828, 42520, 39023,
      55245, 56145, 51119, 60398, 71821, 48142, 60310, 56041, 54176, 66220,
      56336, 55248, 56656, 63305, 54029, 77136, 71902, 71618, 83587, 81068,
      69062, 54848, 53681, 53555,
      50616    // Blocks 2,400,000 to 2,409,999 in July 2021
    };
    const uint64_t block_range_size = 10000;

    uint64_t num_block_sizes = sizeof(average_block_sizes) / sizeof(average_block_sizes[0]);
    uint64_t weight = 0;
    uint64_t table_index = start_block / block_range_size;
    for (;;) {
      if (num_blocks == 0)
      {
        break;
      }
      if (table_index >= num_block_sizes)
      {
        // Take all blocks beyond our table as having the size of the blocks
        // in the last table entry i.e. in the most recent known block range
        weight += num_blocks * average_block_sizes[num_block_sizes - 1];
        break;
      }
      uint64_t portion_size = std::min(num_blocks, block_range_size - start_block % block_range_size);
      weight += portion_size * average_block_sizes[table_index];
      table_index++;
      num_blocks -= portion_size;
      start_block += portion_size;
    }
    return weight;
  }

  std::vector<std::pair<std::string, size_t>> split_string_by_width(const std::string &s, size_t columns)
  {
    std::vector<std::string> lines;
    std::vector<std::pair<std::string, size_t>> all_lines;
    boost::split(lines, s, boost::is_any_of("\n"), boost::token_compress_on);
    for (const auto &e: lines)
    {
      std::vector<std::pair<std::string, size_t>> new_lines = split_line_by_width(e, columns);
      for (auto &l: new_lines)
        all_lines.push_back(std::move(l));
    }
    return all_lines;
  }
}
