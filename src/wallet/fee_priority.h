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
        // If adding or removing an enumeration, ensure to update enums and fee_priority_strings in the class below.
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

        inline constexpr EnumStringsType fee_priority_strings = { { "default", "unimportant", "normal", "elevated", "priority" } };
        inline constexpr EnumsType enums = { { fee_priority::Default, fee_priority::Unimportant, fee_priority::Normal, fee_priority::Elevated, fee_priority::Priority } };

        inline fee_priority decrease(const fee_priority priority)
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

        inline constexpr uint32_t as_integral(const fee_priority priority)
        {
            return static_cast<uint32_t>(priority);
        }

        inline constexpr fee_priority from_integral(const uint32_t priority)
        {
            if (priority >= as_integral(fee_priority::Priority))
            {
                return fee_priority::Priority;
            }

            return static_cast<fee_priority>(priority);
        }

        inline bool is_valid(const uint32_t priority)
        {
            return priority <= as_integral(fee_priority::Priority);
        }

        inline fee_priority clamp(const fee_priority priority)
        {
            const auto highest = as_integral(fee_priority::Priority);
            const auto lowest = as_integral(fee_priority::Default);
            const auto current = as_integral(priority);

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

        inline fee_priority clamp_modified(const fee_priority priority)
        {
            /* Map Default to an actionable priority. */
            if (priority == fee_priority::Default)
            {
                return fee_priority::Unimportant;
            }
            else
            {
                return clamp(priority);
            }
        }

        inline std::string_view to_string(const fee_priority priority)
        {
            const auto integralValue = as_integral(clamp(priority));
            return fee_priority_strings.at(integralValue);
        }

        inline std::optional<fee_priority> from_string(const std::string& str)
        {
            const auto strIterator = std::find(fee_priority_strings.begin(), fee_priority_strings.end(), str);
            if (strIterator == fee_priority_strings.end())
                return std::nullopt;

            const auto distance = std::distance(fee_priority_strings.begin(), strIterator);
            return enums.at(distance);
        }

    }
}
