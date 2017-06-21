#include "readline_buffer.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/select.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>

static int process_input();
static void install_line_handler();
static void remove_line_handler();

static std::string last_line;
static std::string last_prompt;
std::mutex line_mutex, sync_mutex;
std::condition_variable have_line;

namespace
{
  rdln::readline_buffer* current = NULL;
}

rdln::suspend_readline::suspend_readline()
{
  m_buffer = current;
  if(!m_buffer)
    return;
  m_restart = m_buffer->is_running();
  if(m_restart)
    m_buffer->stop();
}

rdln::suspend_readline::~suspend_readline()
{
  if(!m_buffer)
    return;
  if(m_restart)
    m_buffer->start();
}

rdln::readline_buffer::readline_buffer()
: std::stringbuf()
{
  current = this;
}

void rdln::readline_buffer::start()
{
  if(m_cout_buf != NULL)
    return;
  m_cout_buf = std::cout.rdbuf();
  std::cout.rdbuf(this);
  install_line_handler();
}

void rdln::readline_buffer::stop()
{
  if(m_cout_buf == NULL)
    return;
  std::cout.rdbuf(m_cout_buf);
  m_cout_buf = NULL;
  remove_line_handler();
}

void rdln::readline_buffer::get_line(std::string& line) const
{
  std::unique_lock<std::mutex> lock(line_mutex);
  have_line.wait(lock);
  line = last_line;
}

void rdln::readline_buffer::set_prompt(const std::string& prompt)
{
  last_prompt = prompt;
  if(m_cout_buf == NULL)
    return;
  rl_set_prompt(last_prompt.c_str());
  rl_redisplay();
}

int rdln::readline_buffer::process()
{
  if(m_cout_buf == NULL)
    return 0;
  return process_input();
}

int rdln::readline_buffer::sync()
{
  std::lock_guard<std::mutex> lock(sync_mutex);
  char* saved_line;
  int saved_point;
  
  saved_point = rl_point;
  saved_line = rl_copy_text(0, rl_end);
  
  rl_set_prompt("");
  rl_replace_line("", 0);
  rl_redisplay();
  
  do
  {
    char x = this->sgetc();
    m_cout_buf->sputc(x);
  }
  while ( this->snextc() != EOF );
  
  rl_set_prompt(last_prompt.c_str());
  rl_replace_line(saved_line, 0);
  rl_point = saved_point;
  rl_redisplay();
  free(saved_line);
  
  return 0;
}

static fd_set fds;

static int process_input()
{
  int count;
  struct timeval t;
  
  t.tv_sec = 0;
  t.tv_usec = 0;
  
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  count = select(FD_SETSIZE, &fds, NULL, NULL, &t);
  if (count < 1)
  {
    return count;
  }
  rl_callback_read_char();
  return count;
}

static void handle_line(char* line)
{
  if (line != NULL)
  {
    std::lock_guard<std::mutex> lock(sync_mutex);
    rl_set_prompt(last_prompt.c_str());
    rl_already_prompted = 1;
    return;
  }
  rl_set_prompt("");
  rl_replace_line("", 0);
  rl_redisplay();
  rl_set_prompt(last_prompt.c_str());
}

static int handle_enter(int x, int y)
{
  std::lock_guard<std::mutex> lock(sync_mutex);
  char* line = NULL;
  
  line = rl_copy_text(0, rl_end);
  rl_set_prompt("");
  rl_replace_line("", 1);
  rl_redisplay();
  
  if (strcmp(line, "") != 0)
  {
    last_line = line;
    add_history(line);
    have_line.notify_one();
  }
  free(line);
  
  rl_set_prompt(last_prompt.c_str());
  rl_redisplay();
  
  rl_done = 1;
  return 0;
}

static int startup_hook()
{
  rl_bind_key(RETURN, handle_enter);
  rl_bind_key(NEWLINE, handle_enter);
  return 0;
}

static void install_line_handler()
{
  rl_startup_hook = startup_hook;
  rl_callback_handler_install("", handle_line);
}

static void remove_line_handler()
{
  rl_unbind_key(RETURN);
  rl_callback_handler_remove();
}

