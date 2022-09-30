#include "p2p/p2p_protocol_serialization.h"
#include "p2p/portable_scheme/load_store_wrappers_impl.h"

PORTABLE_SCHEME_LOAD_STORE_INSTANCE(nodetool::COMMAND_PING::request)
PORTABLE_SCHEME_LOAD_STORE_INSTANCE(nodetool::COMMAND_PING::response)
PORTABLE_SCHEME_LOAD_STORE_INSTANCE(nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::request)
PORTABLE_SCHEME_LOAD_STORE_INSTANCE(nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::response)
