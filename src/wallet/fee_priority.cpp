#include "fee_priority.h"

namespace tools
{
    std::ostream& operator<<(std::ostream& os, const fee_priority priority)
    {
        return os << fee_priority_utilities::to_string(priority);
    }
}
