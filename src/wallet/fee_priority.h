#pragma once

#include <stdint.h>
#include <string>
#include <string_view>
#include <array>
#include <algorithm>
#include <iterator>
#include <optional>
#include <iosfwd>

namespace tools
{
    enum class fee_priority : uint32_t
    {
        // If adding or removing an enumeation, ensure to update enums_ and feePriorityString_ in the class below.
        Default = 0,
        Unimportant, /* Low */
        Normal, /* Medium */
        Elevated, /* High */
        Priority, /* Very High */
    };

    std::ostream& operator<<(std::ostream& os, const fee_priority priority);

    namespace fee_priority_utilities
    {
        using EnumStringsType = std::array<std::string_view, 5>;
        using EnumsType = std::array<fee_priority, 5>;

        inline constexpr EnumStringsType feePriorityStrings_ = { { "default", "unimportant", "normal", "elevated", "priority" } };
        inline constexpr EnumsType enums_ = { { fee_priority::Default, fee_priority::Unimportant, fee_priority::Normal, fee_priority::Elevated, fee_priority::Priority } };

        static fee_priority Decrease(const fee_priority priority)
        {
            if (priority == fee_priority::Default)
            {
                return fee_priority::Default;
            }
            else
            {
                const uint32_t integralValue = static_cast<uint32_t>(priority);
                const auto decrementedIntegralValue = integralValue - 1u;
                return static_cast<fee_priority>(decrementedIntegralValue);
            }
        }

        static constexpr uint32_t AsIntegral(const fee_priority priority)
        {
            return static_cast<uint32_t>(priority);
        }

        static constexpr fee_priority FromIntegral(const uint32_t priority)
        {
            if (priority >= AsIntegral(fee_priority::Priority))
            {
                return fee_priority::Priority;
            }

            return static_cast<fee_priority>(priority);
        }

        static bool IsValid(const uint32_t priority)
        {
            return priority <= AsIntegral(fee_priority::Priority);
        }

        static fee_priority Clamp(const fee_priority priority)
        {
            const auto highest = AsIntegral(fee_priority::Priority);
            const auto lowest = AsIntegral(fee_priority::Default);
            const auto current = AsIntegral(priority);

            if (current < lowest)
            {
                return fee_priority::Default;
            }
            else if (current > highest)
            {
                return fee_priority::Priority;
            }
            else
            {
                return priority;
            }
        }

        static fee_priority ClampModified(const fee_priority priority)
        {
            /* Map Default to an actionable priority. */
            if (priority == fee_priority::Default)
            {
                return fee_priority::Unimportant;
            }
            else
            {
                return Clamp(priority);
            }
        }

        static std::string_view ToString(const fee_priority priority)
        {
            const auto integralValue = AsIntegral(Clamp(priority));
            return feePriorityStrings_.at(integralValue);
        }

        static std::optional<fee_priority> FromString(const std::string& str)
        {
            const auto strIterator = std::find(feePriorityStrings_.begin(), feePriorityStrings_.end(), str);
            if (strIterator == feePriorityStrings_.end())
                return std::nullopt;

            const auto distance = std::distance(feePriorityStrings_.begin(), strIterator);
            return enums_.at(distance);
        }

    }
}
