// Copyright (c) 2014-2018, The Monero Project
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
#include "is_hdd.h"
#include <cstdlib>
#if defined(__GLIBC__)
  #include <sys/stat.h>
  #include <sstream>
  #include <sys/sysmacros.h>
#elif defined(_WIN32) and (_WIN32_WINNT >= 0x0601)
  #include <windows.h>
  #include <winioctl.h>
  #include <regex>
#endif

namespace tools
{
#if defined(__GLIBC__)
  bool is_hdd_sysfs(const char *file_path, bool &result)
  {
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
      return 0;
    }
    std::string attr_path = prefix + "/queue/rotational";
    FILE *f = fopen(attr_path.c_str(), "r");
    if(f == nullptr)
    {
      attr_path = prefix + "/../queue/rotational";
      f = fopen(attr_path.c_str(), "r");
      if(f == nullptr)
      {
        return 0;
      }
    }
    unsigned short val = 0xdead;
    int r = fscanf(f, "%hu", &val);
    fclose(f);
    if(r == 1)
    {
      result = (val == 1);
      return 1;
    }
    return 0;
  }
#elif defined(_WIN32) and (_WIN32_WINNT >= 0x0601)
  //file path to logical volume
  bool fp2lv(const char *fp, std::string &result)
  {
    HANDLE h = INVALID_HANDLE_VALUE;
    h = CreateFile(
      fp,
      0,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
      nullptr
      );
    if(h != INVALID_HANDLE_VALUE)
    {
      char * p;
      DWORD p_len = GetFinalPathNameByHandleA(
        h,
        nullptr,
        0,
        VOLUME_NAME_NT | FILE_NAME_NORMALIZED
        );
      p = new char[p_len + 1];
      DWORD r_size = GetFinalPathNameByHandleA(
        h,
        p,
        p_len + 1,
        VOLUME_NAME_NT | FILE_NAME_NORMALIZED
        );
      CloseHandle(h);
      p[p_len] = 0;
      std::regex r("^\\\\Device\\\\([^\\\\]+).*$");
      std::smatch m;
      std::string i = p;
      if(std::regex_match(i, m, r))
      {
        delete[] p;
        if(m.size() == 2)
        {
          result = m[1].str();
          return 1;
        }
      }
      else
      {
        delete[] p;
      }
    }
    return 0;
  }

  //logical volume to physical volumes
  bool lv2pv(const char *lv, std::vector<std::string> &pvs)
  {
    HANDLE h = INVALID_HANDLE_VALUE;
    std::string lv_path = "\\\\?\\";
    lv_path += lv;
    h = CreateFile(
      lv_path.c_str(),
      0,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      nullptr
      );
    if(h != INVALID_HANDLE_VALUE)
    {
      VOLUME_DISK_EXTENTS r;
      DWORD r_size = 0;
      BOOL success = DeviceIoControl(
        h,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        nullptr,
        0,
        &r,
        sizeof(r),
        &r_size,
        nullptr
        );
      CloseHandle(h);
      if(success and r_size == sizeof(r))
      {
        for(uint32_t i = 0; i < r.NumberOfDiskExtents; ++i)
        {
          std::ostringstream ss;
          ss << "PhysicalDrive" << r.Extents[i].DiskNumber;
          pvs.push_back(ss.str());
        }
        return 1;
      }
    }
    else
    {
      CloseHandle(h);
    }
    return 0;
  }

  bool win_device_has_seek_penalty(const char *pv, bool &result)
  {
    HANDLE h = INVALID_HANDLE_VALUE;
    std::string pv_path = "\\\\?\\";
    pv_path += pv;
    h = CreateFile(
      pv_path.c_str(),
      0,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      nullptr
      );
    if(h != INVALID_HANDLE_VALUE)
    {
      STORAGE_PROPERTY_QUERY q = {
        .PropertyId = StorageDeviceSeekPenaltyProperty,
        .QueryType = PropertyStandardQuery
      };
      DEVICE_SEEK_PENALTY_DESCRIPTOR r;
      DWORD r_size = 0;
      BOOL success = DeviceIoControl(
        h,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &q,
        sizeof(q),
        &r,
        sizeof(r),
        &r_size,
        nullptr
        );
      CloseHandle(h);
      if(success and r_size == sizeof(r))
      {
        result = r.IncursSeekPenalty;
        return 1;
      }
    }
    else
    {
      CloseHandle(h);
    }
    return 0;
  }

  bool is_hdd_win_ioctl(const char *path, bool &result)
  {
    std::string lv;
    bool lv_success = fp2lv(path, lv);
    if(not lv_success)
    {
      return 0;
    }
    std::vector<std::string> pvs;
    bool pv_success = lv2pv(lv.c_str(), pvs);
    if(not pv_success)
    {
      return 0;
    }
    bool a_result = 0;
    bool a_success = 0;
    for(auto &v: pvs)
    {
      bool q_result;
      bool q_success = win_device_has_seek_penalty(v.c_str(), q_result);
      a_success |= q_success;
      if(q_success)
      {
        a_result |= q_result;
      }
    }
    if(a_success)
    {
      result = a_result;
    }
    return a_success;
  }
#endif
  bool is_hdd(const char *path, bool &result)
  {
    #if defined(_WIN32) and (_WIN32_WINNT >= 0x0601)
    return is_hdd_win_ioctl(path, result);
    #elif defined(__GLIBC__)
    return is_hdd_sysfs(path, result);
    #else
    return 0;
    #endif
  }

  bool is_hdd(const char *path)
  {
    bool result;
    if(is_hdd(path, result))
    {
      return result;
    }
    else
    {
      return 0;
    }
  }
}
