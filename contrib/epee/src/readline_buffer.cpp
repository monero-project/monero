#include "readline_buffer.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/select.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

static int process_input();
static void install_line_handler();
static void remove_line_handler();

static std::string last_line;
static std::string last_prompt;
std::mutex line_mutex, sync_mutex, process_mutex;
std::condition_variable have_line;

std::vector<std::string> rdln::readline_buffer::completion_commands = {"exit"};

namespace
{
  rdln::readline_buffer* current = NULL;
}

rdln::suspend_readline::suspend_readline()
: m_buffer(NULL), m_restart(false)
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
: std::stringbuf(), m_cout_buf(NULL)
{
  current = this;
}

void rdln::readline_buffer::start()
{
  std::unique_lock<std::mutex> lock(process_mutex);
  if(m_cout_buf != NULL)
    return;
  m_cout_buf = std::cout.rdbuf();
  std::cout.rdbuf(this);
  install_line_handler();
}

void rdln::readline_buffer::stop()
{
  std::unique_lock<std::mutex> lock_process(process_mutex);
  std::unique_lock<std::mutex> lock_sync(sync_mutex);
  have_line.notify_all();
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
  std::lock_guard<std::mutex> lock(sync_mutex);
  rl_set_prompt(last_prompt.c_str());
  rl_redisplay();
}

int rdln::readline_buffer::process()
{
  process_mutex.lock();
  if(m_cout_buf == NULL)
  {
    process_mutex.unlock();
    boost::this_thread::sleep_for(boost::chrono::milliseconds( 1 ));
    return 0;
  }
  int count = process_input();
  process_mutex.unlock();
  boost::this_thread::sleep_for(boost::chrono::milliseconds( 1 ));
  return count;
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
    m_cout_buf->sputc( this->sgetc() );
  }
  while ( this->snextc() != EOF );
  
  rl_set_prompt(last_prompt.c_str());
  rl_replace_line(saved_line, 0);
  rl_point = saved_point;
  rl_redisplay();
  free(saved_line);
  
  return 0;
}

static int process_input()
{
  int count;
  struct timeval t;
  fd_set fds;
  
  t.tv_sec = 0;
  t.tv_usec = 1000;
  
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  count = select(STDIN_FILENO + 1, &fds, NULL, NULL, &t);
  if (count < 1)
  {
    return count;
  }
  rl_callback_read_char();
  return count;
}

static void handle_line(char* line)
{
  free(line);
  rl_done = 1;
  return;
}

static int handle_enter(int x, int y)
{
  std::lock_guard<std::mutex> lock(sync_mutex);
  char* line = NULL;

  line = rl_copy_text(0, rl_end);
  std::string test_line = line;
  free(line);
  boost::trim_right(test_line);

  rl_crlf();
  rl_on_new_line();

  if(test_line.empty())
  {
    last_line = "";
    rl_set_prompt(last_prompt.c_str());
    rl_replace_line("", 1);
    rl_redisplay();
    have_line.notify_one();
    return 0;
  }

  rl_set_prompt("");
  rl_replace_line("", 1);
  rl_redisplay();
  
  if (!test_line.empty())
  {
    last_line = test_line;
    add_history(test_line.c_str());
    history_set_pos(history_length);
  }

  if(last_line != "exit" && last_line != "q")
  {
    rl_set_prompt(last_prompt.c_str());
    rl_replace_line("", 1);
    rl_redisplay();
  }

  have_line.notify_one();
  return 0;
}

static int startup_hook()
{
  rl_bind_key(RETURN, handle_enter);
  rl_bind_key(NEWLINE, handle_enter);
  return 0;
}

static char* completion_matches(const char* text, int state)
{
  static size_t list_index;
  static size_t len;

  if(state == 0)
  {
    list_index = 0;
    len = strlen(text);
  }

  const std::vector<std::string>& completions = rdln::readline_buffer::get_completions();
  for(; list_index<completions.size(); )
  {
    const std::string& cmd = completions[list_index++];
    if(cmd.compare(0, len, text) == 0)
    {
      return strdup(cmd.c_str());
    }
  }

  return NULL;
}

static char** attempted_completion(const char* text, int start, int end)
{
  rl_attempted_completion_over = 1;
  return rl_completion_matches(text, completion_matches);
}

static void install_line_handler()
{
  rl_startup_hook = startup_hook;
  rl_attempted_completion_function = attempted_completion;
  rl_callback_handler_install("", handle_line);
  stifle_history(500);
}

static void remove_line_handler()
{
  rl_replace_line("", 0);
  rl_set_prompt("");
  rl_redisplay();
  rl_unbind_key(RETURN);
  rl_unbind_key(NEWLINE);
  rl_callback_handler_remove();
}

