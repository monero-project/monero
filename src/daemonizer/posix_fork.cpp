// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "daemonizer/posix_fork.h"
#include "misc_language.h"
#include "misc_log_ex.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef TMPDIR
#define TMPDIR "/tmp"
#endif

namespace posix {

namespace {
  void quit(const std::string & message)
  {
    LOG_ERROR(message);
    throw std::runtime_error(message);
  }
}

void fork(const std::string & pidfile)
{
  // If a PID file is specified, we open the file here, because
  // we can't report errors after the fork operation.
  // When we fork, we close this file in each of the parent
  // processes.
  // Only in the final child process do we write the PID to the
  // file (and close it).
  int pid_fd = -1;
  auto close_pid_fd = [&pid_fd]()
  {
    if (pid_fd >= 0)
    {
      close(pid_fd);
      pid_fd = -1;
    }
  };
  epee::misc_utils::auto_scope_leave_caller pid_fd_guard =
    epee::misc_utils::create_scope_leave_handler(close_pid_fd);
  if (! pidfile.empty ())
  {
    struct stat st;
    if (lstat(pidfile.c_str(), &st) == 0)
    {
      if (S_ISLNK(st.st_mode))
      {
        quit("PID file path is a symlink, refusing: " + pidfile);
      }
      if (!S_ISREG(st.st_mode))
      {
        quit("PID file path exists and is not a regular file: " + pidfile);
      }

      int oldpid = 0;
      std::ifstream pidrifs;
      pidrifs.open(pidfile, std::fstream::in);
      if (!pidrifs.fail())
      {
        // Read the PID and send signal 0 to see if the process exists.
        errno = 0;
        if (pidrifs >> oldpid && oldpid > 1 && (kill(oldpid, 0) == 0 || errno == EPERM))
        {
          quit("PID file " + pidfile + " already exists and the PID therein is valid");
        }
        pidrifs.close();
      }

      if (unlink(pidfile.c_str()) != 0)
      {
        quit("Failed to remove stale PID file: " + pidfile + ": " + std::strerror(errno));
      }
    }
    else if (errno != ENOENT)
    {
      quit("Failed to inspect PID file path: " + pidfile + ": " + std::strerror(errno));
    }

#ifdef O_NOFOLLOW
    const int flags = O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW;
#else
    const int flags = O_WRONLY | O_CREAT | O_EXCL;
#endif
    pid_fd = open(pidfile.c_str(), flags, 0644);
    if (pid_fd < 0)
    {
      quit("Failed to create PID file: " + pidfile + ": " + std::strerror(errno));
    }
  }
  // Fork the process and have the parent exit. If the process was started
  // from a shell, this returns control to the user. Forking a new process is
  // also a prerequisite for the subsequent call to setsid().
  if (pid_t pid = ::fork())
  {
    if (pid > 0)
    {
      // We're in the parent process and need to exit.
      close_pid_fd();
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
      close_pid_fd();
      exit(0);
    }
    else
    {
      quit("Second fork failed");
    }
  }

  if (pid_fd >= 0)
  {
    const std::string pid = std::to_string(::getpid()) + "\n";
    const ssize_t written = write(pid_fd, pid.data(), pid.size());
    if (written < 0)
    {
      quit("Failed to write PID file: " + pidfile + ": " + std::strerror(errno));
    }
    if (static_cast<size_t>(written) != pid.size())
    {
      quit("Failed to write complete PID file: " + pidfile);
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

#ifdef DEBUG_TMPDIR_LOG
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
#else
  if (open("/dev/null", O_WRONLY) < 0)
  {
    quit("Unable to open /dev/null");
  }
#endif

  // Also send standard error to the same log file.
  if (dup(1) < 0)
  {
    quit("Unable to dup output descriptor");
  }
}

} // namespace posix
