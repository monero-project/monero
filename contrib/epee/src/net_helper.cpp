#include "net/net_helper.h"

namespace epee
{
namespace net_utils
{
	boost::unique_future<boost::asio::ip::tcp::socket>
	direct_connect::operator()(const std::string& addr, const std::string& port, boost::asio::steady_timer& timeout) const
	{
		// Get a list of endpoints corresponding to the server name.
		//////////////////////////////////////////////////////////////////////////
		boost::asio::ip::tcp::resolver resolver(timeout.get_executor());

		boost::system::error_code resolve_error;
	  auto result = resolver.resolve(addr, port, resolve_error);
		if(resolve_error || result.empty()) // Documentation states that successful call is guaranteed to be non-empty
			throw boost::system::system_error{boost::asio::error::fault, "Failed to resolve " + addr};

		//////////////////////////////////////////////////////////////////////////

		struct new_connection
		{
			boost::promise<boost::asio::ip::tcp::socket> result_;
			boost::asio::ip::tcp::socket socket_;

			explicit new_connection(const boost::asio::steady_timer::executor_type& io_service)
			  : result_(), socket_(io_service)
			{}
		};

		const auto shared = std::make_shared<new_connection>(timeout.get_executor());
		timeout.async_wait([shared] (boost::system::error_code error)
		{
			if (error != boost::system::errc::operation_canceled && shared && shared->socket_.is_open())
			{
				shared->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				shared->socket_.close();
			}
		});
		shared->socket_.async_connect(*result.begin(), [shared] (boost::system::error_code error)
		{
			if (shared)
			{
				if (error)
					shared->result_.set_exception(boost::system::system_error{error});
				else
					shared->result_.set_value(std::move(shared->socket_));
			}
		});
		return shared->result_.get_future();
	}
}
}

