#pragma once

#ifdef WIN32

#undef UNICODE
#undef _UNICODE

#include <string>
#include <windows.h>

namespace windows
{
  bool install_service(
      std::string const & service_name
    , std::string const & zombie_argument
    );

  bool uninstall_service(
      std::string const & service_name
    );

  bool start_service(
      std::string const & service_name
    );

  bool stop_service(
      std::string const & service_name
    );
}
#endif
