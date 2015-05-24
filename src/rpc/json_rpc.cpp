#include "daemon_deprecated_rpc.h"
#include <signal.h>
#include <iostream>

static bool execute = true;
void trap(int signal) {
  RPC::DaemonDeprecated::stop();
  execute = false;
}

int main() {
  int res = RPC::DaemonDeprecated::start();
  if (res == RPC::DaemonDeprecated::FAILURE_HTTP_SERVER) {
    std::cerr << "Couldn't start HTTP server\n";
    execute = false;
  } else if (res == RPC::DaemonDeprecated::FAILURE_DAEMON_NOT_RUNNING) {
    std::cerr << "Couldn't connect to daemon\n";
    execute = false;
  }
  signal(SIGINT, &trap);
  while (execute) {

  }
  std::cout << "out!\n";
  return 0;
}
