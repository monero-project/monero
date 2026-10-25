#pragma once

#include <streambuf>
#include <sstream>
#include <string>
#include <vector>

namespace rdln
{
  typedef enum { empty, partial, full } linestatus;
  class readline_buffer : public std::streambuf
  {
  public:
    readline_buffer();
    void start();
    void stop();
    bool is_running() const
    {
      return m_cout_buf != NULL;
    }
    linestatus get_line(std::string& line);
    void set_prompt(const std::string& prompt);
    static void add_completion(const std::string& command);
    static const std::vector<std::string>& get_completions();
    
  protected:
    virtual int sync();
    virtual int_type overflow(int_type c);
    virtual std::streamsize xsputn(const char* s, std::streamsize n);

  private:
    std::streambuf* m_cout_buf;
    std::string m_pending;
    static std::vector<std::string>& completion_commands();
  };
  
  class suspend_readline
  {
  public:
    suspend_readline();
    ~suspend_readline();
  private:
    readline_buffer* m_buffer;
    bool m_restart;
  };

  void clear_screen();
}
