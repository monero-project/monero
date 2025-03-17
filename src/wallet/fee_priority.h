#pragma once

#include <stdint.h>
#include <string>
#include <string_view>
#include <array>
#include <algorithm>
#include <iterator>
#include <optional>

namespace tools
{
    enum class FeePriority : uint32_t
    {
        Default = 0,
        Unimportant, /* Low */
        Normal, /* Medium */
        Elevated, /* High */
        Priority, /* Very High */
    };

    std::ostream& operator<<(std::ostream& os, const FeePriority priority);

    class FeePriorityUtilities
    {
    private:
        using EnumStringsType = std::array<std::string_view, 5>;
        using EnumsType = std::array<FeePriority, 5>;

        static inline const EnumStringsType strings_ = { { "default", "unimportant", "normal", "elevated", "priority" } };
        static inline const EnumsType enums_ = { { FeePriority::Default, FeePriority::Unimportant, FeePriority::Normal, FeePriority::Elevated, FeePriority::Priority } };

    public:
        static FeePriority Decrease(const FeePriority priority)
        {
            if (priority == FeePriority::Default)
            {
                return FeePriority::Default;
            }
            else
            {
                const uint32_t integralValue = AsIntegral(priority);
                const auto decrementedIntegralValue = integralValue - 1u;
                return FromIntegral(decrementedIntegralValue);
            }
        }

        static FeePriority Increase(const FeePriority priority)
        {
            if (priority == FeePriority::Priority)
            {
                return priority;
            }
            else
            {
                const uint32_t integralValue = AsIntegral(priority);
                const auto incrementedIntegralValue = integralValue + 1u;
                return FromIntegral(incrementedIntegralValue);
            }
        }

        static uint32_t AsIntegral(const FeePriority priority)
        {
            return static_cast<uint32_t>(priority);
        }

        static FeePriority FromIntegral(const uint32_t priority)
        {
            return static_cast<FeePriority>(priority);
        }

        static FeePriority Clamp(const FeePriority priority)
        {
            /* Map Default to an actionable priority. */
            if (priority == FeePriority::Default)
            {
                return FeePriority::Unimportant;
            }
            else
            {
                return priority;
            }
        }

        static std::string_view ToString(const FeePriority priority)
        {
            const auto integralValue = AsIntegral(priority);
            return strings_.at(integralValue);
        }

        static std::optional<FeePriority> FromString(const std::string& str)
        {
            const auto strIterator = std::find(strings_.begin(), strings_.end(), str);
            if (strIterator == strings_.end())
                return std::nullopt;

            const auto distance = std::distance(strings_.begin(), strIterator);
            return enums_.at(distance);
        }

        static const EnumStringsType GetFeePriorityStrings() { return strings_; }
        static const EnumsType GetEnums() { return enums_; }
    };
}