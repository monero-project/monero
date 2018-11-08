#ifndef LOKI_INTEGRATION_TEST_HOOKS_H
#define LOKI_INTEGRATION_TEST_HOOKS_H

#define LOKI_ENABLE_INTEGRATION_TEST_HOOKS 0
#if defined(LOKI_ENABLE_INTEGRATION_TEST_HOOKS)

#include <stdint.h>
#include <sstream>
#include <iostream>

namespace loki
{
struct fixed_buffer
{
  static const int SIZE = 8192;
  char data[SIZE];
  int  len;
};

void         use_standard_cout();
void         use_redirected_cout();

enum struct shared_mem_type { default_type, wallet, daemon };
void         init_integration_test_context        (shared_mem_type type);
void         write_to_stdout_shared_mem           (char const *buf, int buf_len, shared_mem_type type = shared_mem_type::default_type);
void         write_to_stdout_shared_mem           (std::string const &input, shared_mem_type type = shared_mem_type::default_type);
fixed_buffer read_from_stdin_shared_mem           (shared_mem_type type = shared_mem_type::default_type);
void         write_redirected_stdout_to_shared_mem(shared_mem_type type = shared_mem_type::default_type);
}; // namespace loki
#endif // LOKI_ENABLE_INTEGRATION_TEST_HOOKS

#endif // LOKI_INTEGRATION_TEST_HOOKS_H

