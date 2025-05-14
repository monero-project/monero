#pragma once

namespace tools
{
    enum class fee_algorithm : int
    {
        Unset = -1,
        PreHardforkV3, /* Original */
        HardforkV3,
        HardforkV5,
        HardforkV8,
    };

    class fee_algorithm_utilities
    {
    public:
        static int AsIntegral(const fee_algorithm algorithm)
        {
            return static_cast<int>(algorithm);
        }
    };
}
