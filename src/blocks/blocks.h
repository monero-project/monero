#ifndef SRC_BLOCKS_BLOCKS_H_
#define SRC_BLOCKS_BLOCKS_H_

#include "cryptonote_config.h"
#include "span.h"

namespace blocks
{
    const epee::span<const unsigned char> GetCheckpointsData(cryptonote::network_type network);
}

#endif /* SRC_BLOCKS_BLOCKS_H_ */
