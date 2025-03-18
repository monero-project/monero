#pragma once

#include <stdint.h>
#include <vector>

namespace tools
{
    struct FeeLevelRange
    {
        double minimum;
        double maximum;
    };

    using FeeLevelRanges = std::vector<FeeLevelRange>;

    struct BlockBacklog
    {
        double fee;
        uint64_t blocks_remaining;
    };

    struct BlockRangeBacklog
    {
        BlockBacklog minimum_fee_backlog;
        BlockBacklog maximum_fee_backlog;

        // Paying a lower fee requires one to wait longer.
        uint64_t GetMaximumBlocksRemaining() const { return minimum_fee_backlog.blocks_remaining; }
        uint64_t GetMinimumBlocksRemaining() const { return maximum_fee_backlog.blocks_remaining; }
        uint64_t GetAverageBlocksRemaining() const { return (GetMaximumBlocksRemaining() - GetMinimumBlocksRemaining()) / 2 + GetMinimumBlocksRemaining(); }
    };

    using BlockRangeBacklogs = std::vector<BlockRangeBacklog>;
}