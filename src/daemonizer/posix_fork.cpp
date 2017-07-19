// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "daemonizer/posix_fork.h"
#include "misc_log_ex.h"

#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

#ifndef TMPDIR
#define TMPDIR "/tmp"
#endif

namespace posix {

namespace {
  void quit(std::string const & message)
  {
    LOG_ERROR(message);
    throw std::runtime_error(message);
  }
}

void fork()
{
  // Fork the process and have the parent exit. If the process was started
  // from a shell, this returns control to the user. Forking a new process is
  // also a prerequisite for the subsequent call to setsid().
  if (pid_t pid = ::fork())
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
      quit("First fork failed");
    }
  }

  // Make the process a new session leader. This detaches it from the
  // terminal.
  setsid();

  // A second fork ensures the process cannot acquire a controlling terminal.
  if (pid_t pid = ::fork())
  {
    if (pid > 0)
    {
      exit(0);
    }
    else
    {
      quit("Second fork failed");
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
    quit("Unable to open /dev/null");
  }

  // Send standard output to a log file.
  const char *tmpdir = getenv("TMPDIR");
  if (!tmpdir)
    tmpdir = TMPDIR;
  std::string output = tmpdir;
  output += "/bitmonero.daemon.stdout.stderr";
  const int flags = O_WRONLY | O_CREAT | O_APPEND;
  const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  if (open(output.c_str(), flags, mode) < 0)
  {
    quit("Unable to open output file: " + output);
  }

  // Also send standard error to the same log file.
  if (dup(1) < 0)
  {
    quit("Unable to dup output descriptor");
  }
}

} // namespace posix
