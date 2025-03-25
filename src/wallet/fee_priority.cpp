#include "fee_priority.h"

namespace tools
{
    std::ostream& operator<<(std::ostream& os, const FeePriority priority)
    {
        return os << FeePriorityUtilities::ToString(priority);
    }
}