// Copyright (c) 2014-2017, The Monero Project
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

#include <cstdio>

#include "include_base_utils.h"
#include "file_io_utils.h"
using namespace epee;

#include "util.h"
#include "cryptonote_config.h"
#include "net/http_client.h"                        // epee::net_utils::...

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#include <strsafe.h>
#else 
#include <sys/utsname.h>
#endif
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <openssl/sha.h>

namespace tools
{
  std::function<void(int)> signal_handler::m_handler;

  std::unique_ptr<std::FILE, tools::close_file> create_private_file(const std::string& name)
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
        return nullptr;
    }

    DWORD sid_size = 0;
    GetTokenInformation(process.get(), TokenOwner, nullptr, 0, std::addressof(sid_size));
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      return nullptr;

    std::unique_ptr<char[]> sid{new char[sid_size]};
    if (!GetTokenInformation(process.get(), TokenOwner, sid.get(), sid_size, std::addressof(sid_size)))
      return nullptr;

    const PSID psid = reinterpret_cast<const PTOKEN_OWNER>(sid.get())->Owner;
    const DWORD daclSize =
      sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD);

    const std::unique_ptr<char[]> dacl{new char[daclSize]};
    if (!InitializeAcl(reinterpret_cast<PACL>(dacl.get()), daclSize, ACL_REVISION))
      return nullptr;

    if (!AddAccessAllowedAce(reinterpret_cast<PACL>(dacl.get()), ACL_REVISION, (READ_CONTROL | FILE_GENERIC_READ | DELETE), psid))
      return nullptr;

    SECURITY_DESCRIPTOR descriptor{};
    if (!InitializeSecurityDescriptor(std::addressof(descriptor), SECURITY_DESCRIPTOR_REVISION))
      return nullptr;

    if (!SetSecurityDescriptorDacl(std::addressof(descriptor), true, reinterpret_cast<PACL>(dacl.get()), false))
      return nullptr;

    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), std::addressof(descriptor), false};
    std::unique_ptr<void, close_handle> file{
      CreateFile(
        name.c_str(),
        GENERIC_WRITE, FILE_SHARE_READ,
        std::addressof(attributes),
        CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY,
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
        return {real_file, tools::close_file{}};
      }
    }
#else
    const int fd = open(name.c_str(), (O_RDWR | O_EXCL | O_CREAT), S_IRUSR);
    if (0 <= fd)
    {
      std::FILE* file = fdopen(fd, "w");
      if (!file)
      {
        close(fd);
      }
      return {file, tools::close_file{}};
    }
#endif
    return nullptr;
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

      if ( osvi.dwMajorVersion == 6 )
      {
        if( osvi.dwMinorVersion == 0 )
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
  utsname un;

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
    namespace fs = boost::filesystem;
    char psz_path[MAX_PATH] = "";

    if(SHGetSpecialFolderPathA(NULL, psz_path, nfolder, iscreate))
    {
      return psz_path;
    }

    LOG_ERROR("SHGetSpecialFolderPathA() failed, could not obtain requested path.");
    return "";
  }
#endif
  
  std::string get_default_data_dir()
  {
    /* Please for the love of god refactor  the ifdefs out of this */

    // namespace fs = boost::filesystem;
    // Windows < Vista: C:\Documents and Settings\Username\Application Data\CRYPTONOTE_NAME
    // Windows >= Vista: C:\Users\Username\AppData\Roaming\CRYPTONOTE_NAME
    // Mac: ~/Library/Application Support/CRYPTONOTE_NAME
    // Unix: ~/.CRYPTONOTE_NAME
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
#ifdef MAC_OSX
    // Mac
    pathRet /= "Library/Application Support";
    config_folder =  (pathRet + "/" + CRYPTONOTE_NAME);
#else
    // Unix
    config_folder = (pathRet + "/." + CRYPTONOTE_NAME);
#endif
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

  std::error_code replace_file(const std::string& replacement_name, const std::string& replaced_name)
  {
    int code;
#if defined(WIN32)
    // Maximizing chances for success
    DWORD attributes = ::GetFileAttributes(replaced_name.c_str());
    if (INVALID_FILE_ATTRIBUTES != attributes)
    {
      ::SetFileAttributes(replaced_name.c_str(), attributes & (~FILE_ATTRIBUTE_READONLY));
    }

    bool ok = 0 != ::MoveFileEx(replacement_name.c_str(), replaced_name.c_str(), MOVEFILE_REPLACE_EXISTING);
    code = ok ? 0 : static_cast<int>(::GetLastError());
#else
    bool ok = 0 == std::rename(replacement_name.c_str(), replaced_name.c_str());
    code = ok ? 0 : errno;
#endif
    return std::error_code(code, std::system_category());
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
  void set_strict_default_file_permissions(bool strict)
  {
#if defined(__MINGW32__) || defined(__MINGW__)
    // no clue about the odd one out
#else
    mode_t mode = strict ? 077 : 0;
    umask(mode);
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

  bool is_local_address(const std::string &address)
  {
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
    boost::split(f0, v0, boost::is_any_of("."));
    boost::split(f1, v1, boost::is_any_of("."));
    while (f0.size() < f1.size())
      f0.push_back("0");
    while (f1.size() < f0.size())
      f1.push_back("0");
    for (size_t i = 0; i < f0.size(); ++i) {
      int f0i = atoi(f0[i].c_str()), f1i = atoi(f1[i].c_str());
      int n = f0i - f1i;
      if (n)
        return n;
    }
    return 0;
  }

  bool sha256sum(const uint8_t *data, size_t len, crypto::hash &hash)
  {
    SHA256_CTX ctx;
    if (!SHA256_Init(&ctx))
      return false;
    if (!SHA256_Update(&ctx, data, len))
      return false;
    if (!SHA256_Final((unsigned char*)hash.data, &ctx))
      return false;
    return true;
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
    SHA256_CTX ctx;
    if (!SHA256_Init(&ctx))
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
      if (!SHA256_Update(&ctx, buf, read_size))
        return false;
      size_left -= read_size;
    }
    f.close();
    if (!SHA256_Final((unsigned char*)hash.data, &ctx))
      return false;
    return true;
  }
}
