#pragma once

#include <streambuf>
#include <sstream>
#include <iostream>

namespace rdln
{
  class readline_buffer : public std::stringbuf
  {
  public:
    readline_buffer();
    void start();
    void stop();
    int process();
    bool is_running()
    {
      return m_cout_buf != NULL;
    }
    void get_line(std::string& line);
    void set_prompt(const std::string& prompt);
    
  protected:
    virtual int sync();
    
  private:
    std::streambuf* m_cout_buf;
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

