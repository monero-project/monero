#define LOKI_ENABLE_INTEGRATION_TEST_HOOKS
#if defined(LOKI_ENABLE_INTEGRATION_TEST_HOOKS)

#if defined _WIN32
#error "Need to implement semaphores for Windows Layer"
#endif

#ifndef LOKI_INTEGRATION_TEST_HOOKS_H
#define LOKI_INTEGRATION_TEST_HOOKS_H

//
// Header
//
#include <stdint.h>
#include <sstream>
#include <iostream>
#include <boost/thread/mutex.hpp>
#include "syncobj.h"

#include "command_line.h"
#include "shoom.h"

namespace loki
{
struct fixed_buffer
{
  static const int SIZE = 32768;
  char data[SIZE];
  int  len;
};

void                     init_integration_test_context        (std::string const &base_name);
void                     deinit_integration_test_context      ();
fixed_buffer             read_from_stdin_shared_mem           ();
void                     write_redirected_stdout_to_shared_mem();
void                     use_standard_cout                    ();
void                     use_redirected_cout                  ();
std::vector<std::string> separate_stdin_to_space_delim_args   (fixed_buffer const *cmd);

extern const command_line::arg_descriptor<std::string, false> arg_integration_test_hardforks_override;
extern const command_line::arg_descriptor<std::string, false> arg_integration_test_shared_mem_name;
extern boost::mutex integration_test_mutex;
extern bool core_is_idle;

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

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

static std::ostringstream  global_redirected_cout;
static std::streambuf     *global_std_cout;
static shoom::Shm         *global_stdin_shared_mem;
static shoom::Shm         *global_stdout_shared_mem;
static sem_t              *global_stdin_semaphore_handle;
static sem_t              *global_stdout_semaphore_handle;
static sem_t              *global_stdout_ready_semaphore;
static sem_t              *global_stdin_ready_semaphore;

namespace loki
{
bool core_is_idle;
const command_line::arg_descriptor<std::string, false> arg_integration_test_hardforks_override = {
  "integration-test-hardforks-override"
, "Specify custom hardfork heights and launch in fakenet mode"
, ""
, false
};

const command_line::arg_descriptor<std::string, false> arg_integration_test_shared_mem_name = {
  "integration-test-shared-mem-name"
, "Specify the shared memory base name for stdin, stdout and semaphore name"
, "loki-default-integration-test-mem-name"
, false
};

boost::mutex integration_test_mutex;

} // namespace loki

std::string global_stdin_semaphore_name;
std::string global_stdout_semaphore_name;
std::string global_stdout_ready_semaphore_name;
std::string global_stdin_ready_semaphore_name;

void loki::use_standard_cout()   { if (!global_std_cout) { global_std_cout = std::cout.rdbuf(); } std::cout.rdbuf(global_std_cout); }
void loki::use_redirected_cout() { if (!global_std_cout) { global_std_cout = std::cout.rdbuf(); } std::cout.rdbuf(global_redirected_cout.rdbuf()); }

void loki::init_integration_test_context(const std::string &base_name)
{
  assert(base_name.size() > 0);

  static bool init = false;
  if (init)
    return;

  const std::string stdin_name            = base_name + "_stdin";
  const std::string stdout_name           = base_name + "_stdout";

  global_stdin_semaphore_name  = "/" + base_name + "_stdin_semaphore";
  global_stdout_semaphore_name = "/" + base_name + "_stdout_semaphore";
  global_stdout_ready_semaphore_name = "/" + base_name + "_stdout_ready_semaphore";
  global_stdin_ready_semaphore_name = "/" + base_name + "_stdin_ready_semaphore";

  static shoom::Shm stdin_shared_mem (stdin_name, fixed_buffer::SIZE);
  static shoom::Shm stdout_shared_mem(stdout_name, fixed_buffer::SIZE);

  init = true;
  global_stdin_shared_mem = &stdin_shared_mem;
  global_stdout_shared_mem = &stdout_shared_mem;
  global_std_cout = std::cout.rdbuf();

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

  global_stdin_semaphore_handle = sem_open(global_stdin_semaphore_name.c_str(), O_CREAT, 0600, 0);
  global_stdout_semaphore_handle = sem_open(global_stdout_semaphore_name.c_str(), O_CREAT, 0600, 0);
  global_stdout_ready_semaphore = sem_open(global_stdout_ready_semaphore_name.c_str(), O_CREAT, 0600, 0);
  global_stdin_ready_semaphore = sem_open(global_stdin_ready_semaphore_name.c_str(), O_CREAT, 0600, 0);

  if (!global_stdin_semaphore_handle)  fprintf(stderr, "Loki Integration Test: Failed to initialise global_stdin_semaphore_handle\n");
  if (!global_stdout_semaphore_handle) fprintf(stderr, "Loki Integration Test: Failed to initialise global_stdout_semaphore_handle\n");
  if (!global_stdout_ready_semaphore) fprintf(stderr, "Loki Integration Test: Failed to initialise global_stdout_ready_semaphore_handle\n");
  if (!global_stdin_ready_semaphore) fprintf(stderr, "Loki Integration Test: Failed to initialise global_stdin_ready_semaphore_handle\n");

  printf("Loki Integration Test: Hooks initialised into shared memory, %s, %s, %s, %s, %s, %s\n",
      stdin_name.c_str(),
      stdout_name.c_str(),
      global_stdin_semaphore_name.c_str(),
      global_stdout_semaphore_name.c_str(),
      global_stdin_ready_semaphore_name.c_str(),
      global_stdout_ready_semaphore_name.c_str());
}

void loki::deinit_integration_test_context()
{
  sem_unlink(global_stdin_semaphore_name.c_str());
  sem_unlink(global_stdout_semaphore_name.c_str());
  sem_unlink(global_stdout_ready_semaphore_name.c_str());
  sem_unlink(global_stdin_ready_semaphore_name.c_str());
}

uint32_t const MSG_MAGIC_BYTES = 0x7428da3f;
void write_to_stdout_shared_mem(char const *buf, int buf_len)
{
  assert(global_stdout_shared_mem);

  int sem_value = 0;
  sem_getvalue(global_stdout_ready_semaphore, &sem_value);

  sem_wait(global_stdout_ready_semaphore);
  {
    char *msg_buf         = reinterpret_cast<char *>(global_stdout_shared_mem->Data());
    int const msg_buf_len = global_stdout_shared_mem->Size();

    char *ptr     = msg_buf;
    int total_len = static_cast<int>(sizeof(MSG_MAGIC_BYTES) + buf_len);
    if (total_len >= msg_buf_len)
    {
      int remain_len = msg_buf_len - sizeof(MSG_MAGIC_BYTES);
      ptr = msg_buf + (buf_len - remain_len);
    }

    memcpy(ptr, (char *)&MSG_MAGIC_BYTES, sizeof(MSG_MAGIC_BYTES));
    ptr += sizeof(MSG_MAGIC_BYTES);

    memcpy(ptr, buf, buf_len);
    msg_buf[total_len] = 0;
  }
  sem_post(global_stdout_semaphore_handle);
}

void write_to_stdout_shared_mem(std::string const &input) { write_to_stdout_shared_mem(input.c_str(), input.size()); }

static char *parse_message(char *msg_buf, int msg_buf_len)
{
  char *ptr      = msg_buf;
  uint32_t magic = *(uint32_t *)ptr;
  if (magic != MSG_MAGIC_BYTES) return nullptr;
  ptr += sizeof(MSG_MAGIC_BYTES);

  assert(ptr < msg_buf + msg_buf_len);
  return ptr;
}

std::vector<std::string> loki::separate_stdin_to_space_delim_args(loki::fixed_buffer const *cmd)
{
  std::vector<std::string> args;
  char const *start = cmd->data;
  for (char const *buf_ptr = start; *buf_ptr; ++buf_ptr)
  {
    if (buf_ptr[0] == ' ')
    {
      std::string result(start, buf_ptr - start);
      start = buf_ptr + 1;
      args.push_back(result);
    }
  }

  if (*start)
  {
    std::string last(start);
    args.push_back(last);
  }

  return args;
}

loki::fixed_buffer loki::read_from_stdin_shared_mem()
{
  boost::unique_lock<boost::mutex> scoped_lock(integration_test_mutex);

  assert(global_stdin_shared_mem);

  char *input = nullptr;
  int input_len = 0;
  for(;;)
  {
    int sem_value = 0;
    sem_getvalue(global_stdin_semaphore_handle, &sem_value);
    sem_wait(global_stdin_semaphore_handle);

    global_stdin_shared_mem->Open();
    input = parse_message(reinterpret_cast<char *>(global_stdin_shared_mem->Data()), global_stdin_shared_mem->Size());
    if (input)
    {
      input_len = strlen(input);
      if (input_len > 0) break;
    }

    sem_post(global_stdin_ready_semaphore);
  }

  fixed_buffer result = {};
  result.len = input_len;
  assert(result.len <= fixed_buffer::SIZE);
  memcpy(result.data, input, result.len);
  result.data[result.len] = 0;
  sem_post(global_stdin_ready_semaphore);
  return result;
}

void loki::write_redirected_stdout_to_shared_mem()
{
  boost::unique_lock<boost::mutex> scoped_lock(integration_test_mutex);

  global_redirected_cout.flush();
  std::string output = global_redirected_cout.str();
  global_redirected_cout.str("");
  global_redirected_cout.clear();

  write_to_stdout_shared_mem(output);

  use_standard_cout();
  std::cout << output << std::endl;
  use_redirected_cout();
}

#endif // LOKI_INTEGRATION_TEST_HOOKS_IMPLEMENTATION
#endif // LOKI_ENABLE_INTEGRATION_TEST_HOOKS

