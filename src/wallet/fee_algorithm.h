#pragma once

namespace tools
{
    enum class FeeAlgorithm : int
    {
        Unset = -1,
        PreHardforkV3, /* Original */
        HardforkV3,
        HardforkV5,
        HardforkV8,
    };

    class FeeAlgorithmUtilities
    {
    public:
        static int AsIntegral(const FeeAlgorithm algorithm)
        {
            return static_cast<int>(algorithm);
        }
    };
}