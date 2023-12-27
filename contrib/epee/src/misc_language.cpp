#include "misc_language.h"

#include <boost/thread.hpp>

namespace epee
{
namespace misc_utils
{
	bool sleep_no_w(long ms )
	{
		boost::this_thread::sleep( 
			boost::get_system_time() + 
			boost::posix_time::milliseconds( std::max<long>(ms,0) ) );

		return true;
	}
}
}