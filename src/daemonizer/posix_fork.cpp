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
  // When we fork, we close thise file in each of the parent
  // processes.
  // Only in the final child process do we write the PID to the
  // file (and close it).
  std::ofstream pidofs;
  if (! pidfile.empty ())
  {
	int oldpid;
    std::ifstream pidrifs;
    pidrifs.open(pidfile, std::fstream::in);
    if (! pidrifs.fail())
    {
	  // Read the PID and send signal 0 to see if the process exists.
	  if (pidrifs >> oldpid && oldpid > 1 && kill(oldpid, 0) == 0)
      {
        quit("PID file " + pidfile + " already exists and the PID therein is valid");
	  }
	  pidrifs.close();
	}

    pidofs.open(pidfile, std::fstream::out | std::fstream::trunc);
    if (pidofs.fail())
    {
      quit("Failed to open specified PID file for writing");
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
      pidofs.close();
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
      pidofs.close();
      exit(0);
    }
    else
    {
      quit("Second fork failed");
    }
  }

  if (! pidofs.fail())
  {
    int pid = ::getpid();
    pidofs << pid << std::endl;
    pidofs.close();
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

  // Also send standard error to the same log file.
  if (dup(1) < 0)
  {
    quit("Unable to dup output descriptor");
  }
#endif
}

} // namespace posix
