// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#include <system_error>
#include <boost/filesystem.hpp>

#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "misc_language.h"
#include "p2p/p2p_protocol_defs.h"

namespace tools
{
  std::string get_default_data_dir();
  std::string get_os_version_string();
  bool create_directories_if_necessary(const std::string& path);
  std::error_code replace_file(const std::string& replacement_name, const std::string& replaced_name);

  inline crypto::hash get_proof_of_trust_hash(const nodetool::proof_of_trust& pot)
  {
    std::string s;
    s.append(reinterpret_cast<const char*>(&pot.peer_id), sizeof(pot.peer_id));
    s.append(reinterpret_cast<const char*>(&pot.time), sizeof(pot.time));
    return crypto::cn_fast_hash(s.data(), s.size());
  }
}
