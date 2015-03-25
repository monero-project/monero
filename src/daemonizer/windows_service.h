#pragma once

#ifdef WIN32

#undef UNICODE
#undef _UNICODE

#include <string>
#include <windows.h>

namespace windows
{
  bool check_admin(bool & result);

  bool ensure_admin(
      std::string const & arguments
    );

  bool install_service(
      std::string const & service_name
    , std::string const & arguments
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
