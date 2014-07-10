#pragma once

#include <memory>
#include <boost/program_options.hpp>

namespace daemonize {

class t_daemon_impl;

class t_daemon final {
public:
  static void init_options(boost::program_options::options_description & option_spec);
private:
  std::unique_ptr<t_daemon_impl> mp_impl;
public:
  t_daemon(
      boost::program_options::variables_map const & vm
    );
  t_daemon(t_daemon && other);
  t_daemon & operator=(t_daemon && other);
  ~t_daemon();

  void run();
  void stop();
};
}
