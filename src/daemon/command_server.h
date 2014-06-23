#pragma once

#include "console_handler.h"
#include "daemon/interactive_command_executor.h"
#include "daemon/command_parser_executor.h"

namespace daemonize {

template <typename T_command_executor>
class t_command_server {
private:
  t_command_parser_executor<T_command_executor> m_parser;
  epee::command_handler m_command_lookup;
public:
  t_command_server(
      T_command_executor && executor
    );

  bool process_command_str(const std::string& cmd);

  bool process_command_vec(const std::vector<std::string>& cmd);

private:
  bool help(const std::vector<std::string>& args);

  std::string get_commands_str();
};

} // namespace daemonize
