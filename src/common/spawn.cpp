// Copyright (c) 2018, The Monero Project
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

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef _WIN32
#include <boost/algorithm/string/join.hpp>
#include <boost/scope_exit.hpp>
#include <windows.h>
#else
#include <sys/wait.h>
#endif

#include "misc_log_ex.h"
#include "util.h"
#include "spawn.h"

namespace tools
{

int spawn(const char *filename, const std::vector<std::string>& args, bool wait)
{
#ifdef _WIN32
  std::string joined = boost::algorithm::join(args, " ");
  char *commandLine = !joined.empty() ? &joined[0] : nullptr;
  STARTUPINFOA si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  if (!CreateProcessA(filename, commandLine, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi))
  {
    MERROR("CreateProcess failed. Error code " << GetLastError());
    return -1;
  }
  
  BOOST_SCOPE_EXIT(&pi)
  {
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
  BOOST_SCOPE_EXIT_END

  if (!wait)
  {
    return 0;
  }

  DWORD result = WaitForSingleObject(pi.hProcess, INFINITE);
  if (result != WAIT_OBJECT_0)
  {
    MERROR("WaitForSingleObject failed. Result " << result << ", error code " << GetLastError());
    return -1;
  }

  DWORD exitCode;
  if (!GetExitCodeProcess(pi.hProcess, &exitCode))
  {
    MERROR("GetExitCodeProcess failed. Error code " << GetLastError());
    return -1;
  }

  MINFO("Child exited with " << exitCode);
  return static_cast<int>(exitCode);
#else
  char **argv = (char**)alloca(sizeof(char*) * (args.size() + 1));
  for (size_t n = 0; n < args.size(); ++n)
    argv[n] = (char*)args[n].c_str();
  argv[args.size()] = NULL;

  pid_t pid = fork();
  if (pid < 0)
  {
    MERROR("Error forking: " << strerror(errno));
    return -1;
  }

  // child
  if (pid == 0)
  {
    tools::closefrom(3);
    close(0);
    char *envp[] = {NULL};
    execve(filename, argv, envp);
    MERROR("Failed to execve: " << strerror(errno));
    return -1;
  }

  // parent
  if (pid > 0)
  {
    if (!wait)
      return 0;

    while (1)
    {
      int wstatus = 0;
      pid_t w = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
      if (w  < 0) {
        MERROR("Error waiting for child: " << strerror(errno));
        return -1;
      }
      if (WIFEXITED(wstatus))
      {
        MINFO("Child exited with " << WEXITSTATUS(wstatus));
        return WEXITSTATUS(wstatus);
      }
      if (WIFSIGNALED(wstatus))
      {
        MINFO("Child killed by " << WEXITSTATUS(wstatus));
        return WEXITSTATUS(wstatus);
      }
    }
  }
  MERROR("Secret passage found");
  return -1;
#endif
}

}
