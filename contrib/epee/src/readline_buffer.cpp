#include "readline_buffer.h"
#include "memwipe.h"
#include <linenoise.h>
#include <algorithm>
#include <iostream>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <thread>
#include <system_error>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/algorithm/string.hpp>

namespace
{
  rdln::readline_buffer* current = NULL;

  std::string strip_unsafe_bytes(const std::string& line)
  {
    std::string out;
    out.reserve(line.size());
    for (size_t i = 0; i < line.size(); )
    {
      const unsigned char c = static_cast<unsigned char>(line[i]);
      if (c == 0xC2 && i + 1 < line.size())
      {
        const unsigned char c1 = static_cast<unsigned char>(line[i + 1]);
        if (c1 >= 0x80 && c1 <= 0x9F) { i += 2; continue; } // UTF-8 encoded C1 control
      }
      if ((c < 0x20 && c != '\n') || c == 0x7F) { ++i; continue; } // C0 (keep \n) / DEL
      out += line[i];
      ++i;
    }
    return out;
  }

  // Wipe old buffer storage when growing, as console output may contain private data
  void append_wiped(std::string& buf, const char* s, size_t n)
  {
    if (buf.size() + n > buf.capacity())
    {
      std::string grown;
      grown.reserve(std::max(buf.capacity() * 2, buf.size() + n));
      grown.assign(buf);
      memwipe(&buf[0], buf.size());
      buf.swap(grown);
    }
    buf.append(s, n);
  }
}

static const size_t LINE_BUFFER_MAX = 1024 * 1024;

static boost::mutex sync_mutex;
static linenoiseState line_state;
static bool editing_active = false;
static bool tty_editing = false;
static std::string current_prompt;

static void completion_callback(const char* buf, linenoiseCompletions* lc)
{
  const std::string text(buf);

  // Completing an empty line would let TAB, Enter run "exit".
  if (text.empty() || text.find(' ') != std::string::npos) return;

  for (const std::string& cmd : rdln::readline_buffer::get_completions())
  {
    if (cmd.compare(0, text.size(), text) == 0)
      linenoiseAddCompletion(lc, (cmd + " ").c_str());
  }
}

// linenoise only reads the terminal width when editing starts.
static void update_columns()
{
  struct winsize ws;
  if (tty_editing && ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col)
    line_state.cols = ws.ws_col;
}

static void begin_edit()
{
  if (editing_active) return;

  char* buf = line_state.buf;
  size_t buflen = line_state.buflen + 1;
  bool allocated = false;
  if (!buf)
  {
    buflen = 4096;
    buf = static_cast<char*>(malloc(buflen));
    if (!buf) return;
    allocated = true;
  }

  if (linenoiseEditStart(&line_state, -1, -1, buf, buflen, current_prompt.c_str()) == 0)
  {
    editing_active = true;
    line_state.buflen_max = LINE_BUFFER_MAX;
  }
  else if (allocated)
  {
    free(buf);
  }
}

static void end_edit()
{
  if (!editing_active) return;
  linenoiseEditStop(&line_state);
  editing_active = false;
}

static void reset_line()
{
  // A non-tty session never drew a prompt, so skip the redraw.
  if (editing_active && tty_editing && write(line_state.ofd, "\n", 1) == -1) {}
  line_state.in_completion = 0;
  line_state.completion_idx = 0;
  line_state.buf[0] = '\0';
  line_state.pos = 0;
  line_state.oldpos = 0;
  line_state.len = 0;
  line_state.oldrows = 0;
  line_state.oldrpos = 1;
  line_state.history_index = 0;
  line_state.fold_count = 0;
  if (editing_active && tty_editing)
  {
    linenoiseShow(&line_state);
  }
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

std::vector<std::string>& rdln::readline_buffer::completion_commands()
{
  static std::vector<std::string> commands = {"exit"};
  return commands;
}

rdln::readline_buffer::readline_buffer()
: std::streambuf(), m_cout_buf(NULL)
{
  current = this;
}

void rdln::readline_buffer::start()
{
  boost::lock_guard<boost::mutex> lock(sync_mutex);
  if(m_cout_buf != NULL)
    return;
  static bool stdin_unbuffered = false;
  if (!stdin_unbuffered && !isatty(STDIN_FILENO))
  {
    setvbuf(stdin, NULL, _IONBF, 0);
    stdin_unbuffered = true;
  }
  // Mirrors linenoiseEditStart()'s own plain-input check.
  tty_editing = !linenoiseIsUnsupportedTerm() &&
    ((isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) || getenv("LINENOISE_ASSUME_TTY"));
  m_cout_buf = std::cout.rdbuf();
  std::cout.rdbuf(this);
  linenoiseSetCompletionCallback(&completion_callback);
  linenoiseHistorySetMaxLen(500);
  begin_edit();
}

void rdln::readline_buffer::stop()
{
  boost::lock_guard<boost::mutex> lock(sync_mutex);
  if(m_cout_buf == NULL)
    return;
  std::cout.rdbuf(m_cout_buf);
  m_cout_buf = NULL;
  if (editing_active && tty_editing)
    linenoiseHide(&line_state);
  end_edit();
}

rdln::linestatus rdln::readline_buffer::get_line(std::string& line)
{
  boost::unique_lock<boost::mutex> lock(sync_mutex);
  if (!m_cout_buf)
  {
    line = "";
    return rdln::full;
  }

  begin_edit();
  if (!editing_active)
  {
    // Raw mode could not be set up: don't spin on the input left unread.
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return rdln::partial;
  }

  // Avoid blocking in read() while holding sync_mutex: another thread
  // may have already consumed the byte that made select() wake us.
  struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
  if (poll(&pfd, 1, 0) <= 0)
    return rdln::partial;

  update_columns();
  errno = 0;
  char* result = linenoiseEditFeed(&line_state);

  if (result == linenoiseEditMore)
    return rdln::partial;

  if (result == NULL && errno == EINTR)
  {
    return rdln::partial;
  }

  const bool cancelled = (result == NULL && errno == EAGAIN);

  if (result == NULL)
  {
    if (!cancelled)
    {
      end_edit();
      return rdln::empty;
    }
    reset_line();
    try { std::thread([]{ raise(SIGINT); }).detach(); }
    catch (const std::system_error&) { raise(SIGINT); }
    return rdln::partial;
  }

  line = strip_unsafe_bytes(result);
  std::string test_line = line;
  boost::trim_right(test_line);
  if (!test_line.empty())
    linenoiseHistoryAdd(test_line.c_str());
  linenoiseFree(result);
  reset_line();
  return rdln::full;
}

void rdln::readline_buffer::set_prompt(const std::string& prompt)
{
  boost::lock_guard<boost::mutex> lock(sync_mutex);
  if(m_cout_buf == NULL)
    return;
  if (editing_active && tty_editing)
    linenoiseHide(&line_state);
  current_prompt = tty_editing ? "\033[1;33m" + prompt + "\033[0m" : prompt;
  if (editing_active)
  {
    line_state.prompt = current_prompt.c_str();
    line_state.plen = current_prompt.size();
    if (tty_editing) linenoiseShow(&line_state);
  }
  else
  {
    begin_edit();
  }

  // linenoise draws no prompt on an unsupported terminal, so print it here.
  if (!tty_editing && isatty(STDIN_FILENO) && isatty(STDOUT_FILENO))
  {
    m_cout_buf->sputn(prompt.data(), prompt.size());
    m_cout_buf->pubsync();
  }
}

void rdln::readline_buffer::add_completion(const std::string& command)
{
  if(std::find(completion_commands().begin(), completion_commands().end(), command) != completion_commands().end())
    return;
  completion_commands().push_back(command);
}

const std::vector<std::string>& rdln::readline_buffer::get_completions()
{
  return completion_commands();
}

int rdln::readline_buffer::sync()
{
  boost::lock_guard<boost::mutex> lock(sync_mutex);

  if (m_cout_buf == nullptr)
  {
    return -1;
  }

  update_columns();
  if (editing_active && tty_editing)
    linenoiseHide(&line_state);

  m_cout_buf->sputn(m_pending.data(), m_pending.size());
  memwipe(&m_pending[0], m_pending.size());
  m_pending.clear();

  m_cout_buf->pubsync();

  if (editing_active && tty_editing)
    linenoiseShow(&line_state);

  return 0;
}

rdln::readline_buffer::int_type rdln::readline_buffer::overflow(int_type c)
{
  if (traits_type::eq_int_type(c, traits_type::eof()))
    return traits_type::not_eof(c);
  const char ch = traits_type::to_char_type(c);
  boost::lock_guard<boost::mutex> lock(sync_mutex);
  append_wiped(m_pending, &ch, 1);
  return c;
}

std::streamsize rdln::readline_buffer::xsputn(const char* s, std::streamsize n)
{
  boost::lock_guard<boost::mutex> lock(sync_mutex);
  append_wiped(m_pending, s, n);
  return n;
}

void rdln::clear_screen()
{
  linenoiseClearScreen();
}
