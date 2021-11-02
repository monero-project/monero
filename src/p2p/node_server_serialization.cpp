#include "p2p/p2p_protocol_serialization.h"
#include "cryptonote_protocol/cryptonote_protocol_serialization.h"
#include "p2p/portable_scheme/load_store_wrappers_impl.h"

PORTABLE_SCHEME_LOAD_STORE_INSTANCE(nodetool::COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::request)
PORTABLE_SCHEME_LOAD_STORE_INSTANCE(nodetool::COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::response)
PORTABLE_SCHEME_LOAD_STORE_INSTANCE(nodetool::COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::request)
PORTABLE_SCHEME_LOAD_STORE_INSTANCE(nodetool::COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::response)
