// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "checkpoints.h"
#include "misc_log_ex.h"

#define ADD_CHECKPOINT(h, hash)  CHECK_AND_ASSERT(checkpoints.add_checkpoint(h,  hash), false);

namespace cryptonote {
  inline bool create_checkpoints(cryptonote::checkpoints& checkpoints)
  {
    ADD_CHECKPOINT(79000,  "cae33204e624faeb64938d80073bb7bbacc27017dc63f36c5c0f313cad455a02");
    ADD_CHECKPOINT(140000, "993059fb6ab92db7d80d406c67a52d9c02d873ca34b6290a12b744c970208772");
    ADD_CHECKPOINT(200000, "a5f74c7542077df6859f48b5b1f9c3741f29df38f91a47e14c94b5696e6c3073");
    ADD_CHECKPOINT(230580, "32bd7cb6c68a599cf2861941f29002a5e203522b9af54f08dfced316f6459103");
    ADD_CHECKPOINT(260000, "f68e70b360ca194f48084da7a7fd8e0251bbb4b5587f787ca65a6f5baf3f5947");
    ADD_CHECKPOINT(300000, "8e80861713f68354760dc10ea6ea79f5f3ff28f39b3f0835a8637463b09d70ff");
    ADD_CHECKPOINT(390285, "e00bdc9bf407aeace2f3109de11889ed25894bf194231d075eddaec838097eb7");
    ADD_CHECKPOINT(417000, "2dc96f8fc4d4a4d76b3ed06722829a7ab09d310584b8ecedc9b578b2c458a69f");
    ADD_CHECKPOINT(427193, "00feabb08f2d5759ed04fd6b799a7513187478696bba2db2af10d4347134e311");
    ADD_CHECKPOINT(453537, "d17de6916c5aa6ffcae575309c80b0f8fdcd0a84b5fa8e41a841897d4b5a4e97");

    return true;
  }
}
