#define LOKI_ENABLE_INTEGRATION_TEST_HOOKS
#if defined(LOKI_ENABLE_INTEGRATION_TEST_HOOKS)

#ifndef LOKI_INTEGRATION_TEST_HOOKS_H
#define LOKI_INTEGRATION_TEST_HOOKS_H

//
// Header
//
#include <stdint.h>
#include <sstream>
#include <iostream>

#include "command_line.h"
#include "shoom.h"

namespace loki
{
struct fixed_buffer
{
  static const int SIZE = 8192;
  char data[SIZE];
  int  len;
};

void         init_integration_test_context        (shoom::Shm *stdin_shared_mem, shoom::Shm *stdout_shared_mem);
void         write_to_stdout_shared_mem           (char const *buf, int buf_len);
void         write_to_stdout_shared_mem           (std::string const &input);
fixed_buffer read_from_stdin_shared_mem           ();
void         write_redirected_stdout_to_shared_mem();
void         use_standard_cout                    ();
void         use_redirected_cout                  ();

extern const command_line::arg_descriptor<std::string, false> arg_integration_test_hardforks_override;
extern const command_line::arg_descriptor<std::string, false> arg_integration_test_shared_mem_stdin;
extern const command_line::arg_descriptor<std::string, false> arg_integration_test_shared_mem_stdout;

}; // namespace loki

#endif // LOKI_INTEGRATION_TEST_HOOKS_H

//
// CPP Implementation
//
#ifdef LOKI_INTEGRATION_TEST_HOOKS_IMPLEMENTATION
#include <string.h>
#include <assert.h>
#include <chrono>
#include <thread>

#define SHOOM_IMPLEMENTATION
#include "shoom.h"

static std::ostringstream  global_redirected_cout;
static std::streambuf     *global_std_cout;
static shoom::Shm         *global_stdin_shared_mem;
static shoom::Shm         *global_stdout_shared_mem;

namespace loki
{
const command_line::arg_descriptor<std::string, false> arg_integration_test_hardforks_override = {
  "integration-test-hardforks-override"
, "Specify custom hardfork heights and launch in fakenet mode"
, ""
, false
};

const command_line::arg_descriptor<std::string, false> arg_integration_test_shared_mem_stdin = {
  "integration-test-shared-mem-stdin"
, "Specify the shared memory name for redirecting stdin"
, "loki-default-integration-test-mem-stdin"
, false
};
const command_line::arg_descriptor<std::string, false> arg_integration_test_shared_mem_stdout = {
  "integration-test-shared-mem-stdout"
, "Specify the shared memory name for redirecting stdout"
, "loki-default-integration-test-mem-stdout"
, false
};
} // namespace loki

void loki::use_standard_cout()   { if (!global_std_cout) { global_std_cout = std::cout.rdbuf(); } std::cout.rdbuf(global_std_cout); }
void loki::use_redirected_cout() { if (!global_std_cout) { global_std_cout = std::cout.rdbuf(); } std::cout.rdbuf(global_redirected_cout.rdbuf()); }

void loki::init_integration_test_context(shoom::Shm *stdin_shared_mem, shoom::Shm *stdout_shared_mem)
{
  static bool init = false;
  if (init)
    return;

  init                     = true;
  global_stdin_shared_mem  = stdin_shared_mem;
  global_stdout_shared_mem = stdout_shared_mem;
  global_std_cout          = std::cout.rdbuf();

  global_stdout_shared_mem->Create(shoom::Flag::create);
  while (global_stdin_shared_mem->Open() != 0)
  {
    static bool once_only = true;
    if (once_only)
    {
      once_only = false;
      printf("Loki Integration Test: Shared memory %s has not been created yet, blocking ...\n", global_stdin_shared_mem->Path().c_str());
    }
  }

  printf("Loki Integration Test: Hooks initialised into shared memory stdin/stdout\n");
}

uint32_t const MSG_MAGIC_BYTES = 0x7428da3f;
static void make_message(char *msg_buf, int msg_buf_len, char const *msg_data, int msg_data_len)
{
  static int32_t cmd_index = 0;
  cmd_index++;

  int total_len             = static_cast<int>(sizeof(cmd_index) + sizeof(MSG_MAGIC_BYTES) + msg_data_len);
  assert(total_len < msg_buf_len);

  char *ptr = msg_buf;
  memcpy(ptr, &cmd_index, sizeof(cmd_index));
  ptr += sizeof(cmd_index);

  memcpy(ptr, (char *)&MSG_MAGIC_BYTES, sizeof(MSG_MAGIC_BYTES));
  ptr += sizeof(MSG_MAGIC_BYTES);

  memcpy(ptr, msg_data, msg_data_len);
  ptr += sizeof(msg_data);

  msg_buf[total_len] = 0;
}

static char const *parse_message(char const *msg_buf, int msg_buf_len, uint32_t *cmd_index)
{
  char const *ptr = msg_buf;
  *cmd_index = *((decltype(cmd_index))ptr);
  ptr += sizeof(*cmd_index);

  if ((*(decltype(MSG_MAGIC_BYTES) const *)ptr) != MSG_MAGIC_BYTES)
    return nullptr;

  ptr += sizeof(MSG_MAGIC_BYTES);
  assert(ptr < msg_buf + msg_buf_len);
  return ptr;
}

void loki::write_to_stdout_shared_mem(char const *buf, int buf_len)
{
  assert(global_stdout_shared_mem);
  make_message(reinterpret_cast<char *>(global_stdout_shared_mem->Data()), global_stdout_shared_mem->Size(), buf, buf_len);
}

void loki::write_to_stdout_shared_mem(std::string const &input) { write_to_stdout_shared_mem(input.c_str(), input.size()); }

loki::fixed_buffer loki::read_from_stdin_shared_mem()
{
  assert(global_stdin_shared_mem);
  static uint32_t last_cmd_index = 0;
  char const *input              = nullptr;

  for (;;)
  {
    if (global_stdin_shared_mem->Open() == shoom::ShoomError::kOK)
    {
      char const *data   = reinterpret_cast<char const *>(global_stdin_shared_mem->Data());
      uint32_t cmd_index = 0;
      input              = parse_message(data, global_stdin_shared_mem->Size(), &cmd_index);

      if (input && last_cmd_index < cmd_index)
      {
        last_cmd_index = cmd_index;
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  fixed_buffer result = {};
  result.len = strlen(input);
  assert(result.len <= fixed_buffer::SIZE);
  memcpy(result.data, input, result.len);
  result.data[result.len] = 0;
  return result;
}

void loki::write_redirected_stdout_to_shared_mem()
{
  std::string output = global_redirected_cout.str();
  global_redirected_cout.flush();
  global_redirected_cout.str("");
  global_redirected_cout.clear();
  loki::write_to_stdout_shared_mem(output);

  loki::use_standard_cout();
  std::cout << output << std::endl;
  loki::use_redirected_cout();
}

#endif // LOKI_INTEGRATION_TEST_HOOKS_IMPLEMENTATION
#endif // LOKI_ENABLE_INTEGRATION_TEST_HOOKS

