#include "data_logger.hpp"

#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>
#include <chrono>
#include "../../contrib/otshell_utils/utils.hpp"

namespace epee
{
namespace net_utils
{
	data_logger &data_logger::get_instance()
	{
		static data_logger instance;
		return instance;
	}
	
	data_logger::data_logger()
	{
		//create timer
		std::shared_ptr<std::thread> logger_thread(new std::thread([&]()
		{
			while (true)
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
				saveToFile();
			}
		}));
		logger_thread->detach();
		
		mFilesMap["peers"] = data_logger::fileData("log/dr-monero/peers.data");
		mFilesMap["download"] = data_logger::fileData("log/dr-monero/net/in-all.data");
		mFilesMap["upload"] = data_logger::fileData("log/dr-monero/net/out-all.data");
		mFilesMap["request"] = data_logger::fileData("log/dr-monero/net/req-all.data");
		mFilesMap["sleep_down"] = data_logger::fileData("log/dr-monero/down_sleep_log.data");
		mFilesMap["sleep_up"] = data_logger::fileData("log/dr-monero/up_sleep_log.data");
		mFilesMap["calc_time"] = data_logger::fileData("log/dr-monero/get_objects_calc_time.data");
		mFilesMap["blockchain_processing_time"] = data_logger::fileData("log/dr-monero/blockchain_log.data");
		
		mFilesMap["peers_limit"] = data_logger::fileData("log/dr-monero/peers_limit.info");
		mFilesMap["download_limit"] = data_logger::fileData("log/dr-monero/limit_down.info");
		mFilesMap["upload_limit"] = data_logger::fileData("log/dr-monero/limit_up.info");
		
		mFilesMap["peers_limit"].mLimitFile = true;
		mFilesMap["download_limit"].mLimitFile = true;
		mFilesMap["upload_limit"].mLimitFile = true;
	}
	
	void data_logger::add_data(std::string filename, unsigned int data)
	{
		if (mFilesMap.find(filename) == mFilesMap.end())
			return; // TODO: exception
			
		
		nOT::nUtils::cFilesystemUtils::CreateDirTree("log/dr-monero/net/");
		
		std::lock_guard<std::mutex> lock(mSaveMutex);
		if (mFilesMap[filename].mLimitFile)
			mFilesMap[filename].mDataToSave = data;
		else
			mFilesMap[filename].mDataToSave += data;
	}
	
	double data_logger::fileData::get_current_time()
	{
		using namespace boost::chrono;
		auto point = steady_clock::now();
		auto time_from_epoh = point.time_since_epoch();
		auto ms = duration_cast< milliseconds >( time_from_epoh ).count();
		double ms_f = ms;
		return ms_f / 1000.;
	}
	
	data_logger::fileData::fileData(std::string pFile)
	{
		mFile = std::make_shared<std::ofstream> (pFile);
		mPath = pFile;
	}
	
	void data_logger::fileData::save()
	{
		if (!data_logger::m_save_graph)
			return;
		mFile->open(mPath, std::ios::app);
		*mFile << static_cast<int>(get_current_time()) << " " << mDataToSave << std::endl;
		mFile->close();
	}
	
	void data_logger::saveToFile()
	{
		std::lock_guard<std::mutex> lock(mSaveMutex);
		for (auto &element : mFilesMap)
		{
			element.second.save();
			if (!element.second.mLimitFile)
				element.second.mDataToSave = 0;
		}
	}
	
std::atomic<bool> data_logger::m_save_graph(false);

} // namespace
} // namespace
