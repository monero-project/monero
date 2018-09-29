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
#include <sys/wait.h>

#include "misc_log_ex.h"
#include "exec.h"

namespace tools
{

int exec(const char *filename, char * const argv[], bool wait)
{
  pid_t pid = fork();
  if (pid < 0)
  {
    MERROR("Error forking: " << strerror(errno));
    return -1;
  }

  // child
  if (pid == 0)
  {
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
}

}
