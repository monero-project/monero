// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "p2p/net_node_common.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "cryptonote_core/connection_context.h"
namespace cryptonote
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct i_cryptonote_protocol
  {
    virtual bool relay_block(NOTIFY_NEW_BLOCK::request& arg, cryptonote_connection_context& exclude_context)=0;
    virtual bool relay_transactions(NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& exclude_context)=0;
    //virtual bool request_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, cryptonote_connection_context& context)=0;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct cryptonote_protocol_stub: public i_cryptonote_protocol
  {
    virtual bool relay_block(NOTIFY_NEW_BLOCK::request& arg, cryptonote_connection_context& exclude_context)
    {
      return false;
    }
    virtual bool relay_transactions(NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& exclude_context)
    {
      return false;
    }

  };
}
