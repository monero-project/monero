#include "scoped_message_writer.h"

#define LOKI_INTEGRATION_TEST_HOOKS_IMPLEMENTATION
#include "common/loki_integration_test_hooks.h"

// NOTE(loki): This file only exists because I need a way to hook into the
// message writer for integration tests. Originally this was a header only file,
// which means it needs to know the implementation of the integration test
// functions which isn't possible to expose in just the header because of the
// One Definition Rule.
//   - doyle 2018-11-08

tools::scoped_message_writer::~scoped_message_writer()
{
  if (m_flush)
  {
    m_flush = false;

#if defined(LOKI_ENABLE_INTEGRATION_TEST_HOOKS) && 0
    loki::use_redirected_cout();
    std::cout << m_oss.str();
    loki::use_standard_cout();
    loki::write_redirected_stdout_to_shared_mem();
#endif

    MCLOG_FILE(m_log_level, "msgwriter", m_oss.str());
    if (epee::console_color_default == m_color)
    {
      std::cout << m_oss.str();
    }
    else
    {
#if !defined(LOKI_ENABLE_INTEGRATION_TEST_HOOKS)
      PAUSE_READLINE();
#endif
      set_console_color(m_color, m_bright);
      std::cout << m_oss.str();
      epee::reset_console_color();
    }
    std::cout << std::endl;
  }
}
