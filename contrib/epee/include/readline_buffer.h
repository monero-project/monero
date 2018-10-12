#pragma once

#include <streambuf>
#include <sstream>
#include <vector>

namespace rdln
{
  typedef enum { empty, partial, full } linestatus;
  class readline_buffer : public std::stringbuf
  {
  public:
    readline_buffer();
    void start();
    void stop();
    bool is_running() const
    {
      return m_cout_buf != NULL;
    }
    linestatus get_line(std::string& line) const;
    void set_prompt(const std::string& prompt);
    static void add_completion(const std::string& command);
    static const std::vector<std::string>& get_completions();
    
  protected:
    virtual int sync();

  private:
    std::streambuf* m_cout_buf;
    size_t m_prompt_length;
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
}

