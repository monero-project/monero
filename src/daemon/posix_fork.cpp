// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "daemon/posix_fork.h"
#include "misc_log_ex.h"

#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace daemonize {

void posix_fork()
{
  // Fork the process and have the parent exit. If the process was started
  // from a shell, this returns control to the user. Forking a new process is
  // also a prerequisite for the subsequent call to setsid().
  if (pid_t pid = fork())
  {
    if (pid > 0)
    {
      // We're in the parent process and need to exit.
      //
      // When the exit() function is used, the program terminates without
      // invoking local variables' destructors. Only global variables are
      // destroyed.
      exit(0);
    }
    else
    {
      LOG_ERROR("First fork failed: %m");
      exit(1);
    }
  }

  // Make the process a new session leader. This detaches it from the
  // terminal.
  setsid();

  // A process inherits its working directory from its parent. This could be
  // on a mounted filesystem, which means that the running daemon would
  // prevent this filesystem from being unmounted. Changing to the root
  // directory avoids this problem.
  if (chdir("/") < 0)
  {
    LOG_ERROR("Unable to change working directory to root");
    exit(1);
  }

  // The file mode creation mask is also inherited from the parent process.
  // We don't want to restrict the permissions on files created by the
  // daemon, so the mask is cleared.
  umask(0);

  // A second fork ensures the process cannot acquire a controlling terminal.
  if (pid_t pid = fork())
  {
    if (pid > 0)
    {
      exit(0);
    }
    else
    {
      LOG_ERROR("Second fork failed");
      exit(1);
    }
  }

  // Close the standard streams. This decouples the daemon from the terminal
  // that started it.
  close(0);
  close(1);
  close(2);

  // We don't want the daemon to have any standard input.
  if (open("/dev/null", O_RDONLY) < 0)
  {
    LOG_ERROR("Unable to open /dev/null");
    exit(1);
  }

  // Send standard output to a log file.
  const char* output = "/tmp/bitmonero.daemon.stdout.stderr";
  const int flags = O_WRONLY | O_CREAT | O_APPEND;
  const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  if (open(output, flags, mode) < 0)
  {
    LOG_ERROR("Unable to open output file: " << output);
    exit(1);
  }

  // Also send standard error to the same log file.
  if (dup(1) < 0)
  {
    LOG_ERROR("Unable to dup output descriptor");
    exit(1);
  }
}

}
