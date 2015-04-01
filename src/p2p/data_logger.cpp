#include "data_logger.hpp"
#include <stdexcept>

#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>
#include <chrono>
#include "../../contrib/otshell_utils/utils.hpp"

namespace epee
{
namespace net_utils
{
	data_logger &data_logger::get_instance() {
		std::call_once(m_singleton, 
			[] { 
				_info_c("dbg/data","Creating singleton of data_logger");
				if (m_state != data_logger_state::state_before_init) { _erro_c("dbg/data","Internal error in singleton"); throw std::runtime_error("data_logger singleton"); }
				m_state = data_logger_state::state_during_init;
				m_obj.reset(new data_logger());
				m_state = data_logger_state::state_ready_to_use;
			}	
		);
		
		if (m_state != data_logger_state::state_ready_to_use) {
			_erro ("trying to use not working data_logger");
			throw std::runtime_error("data_logger ctor state");
		}
		
		return * m_obj;
	}
	
	data_logger::data_logger() {
		_warn_c("dbg/data","Starting data logger (for graphs data)");
		if (m_state != data_logger_state::state_during_init) { _erro_c("dbg/data","Singleton ctor state"); throw std::runtime_error("data_logger ctor state"); }
		std::lock_guard<std::mutex> lock(mMutex); // lock
		
		// prepare all the files for given data channels:
		mFilesMap["peers"] = data_logger::fileData("log/dr-monero/peers.data");
		mFilesMap["download"] = data_logger::fileData("log/dr-monero/net/in-all.data");
		mFilesMap["upload"] = data_logger::fileData("log/dr-monero/net/out-all.data");
		mFilesMap["request"] = data_logger::fileData("log/dr-monero/net/req-all.data");
		mFilesMap["sleep_down"] = data_logger::fileData("log/dr-monero/down_sleep_log.data");
		mFilesMap["sleep_up"] = data_logger::fileData("log/dr-monero/up_sleep_log.data");
		mFilesMap["calc_time"] = data_logger::fileData("log/dr-monero/get_objects_calc_time.data");
		mFilesMap["blockchain_processing_time"] = data_logger::fileData("log/dr-monero/blockchain_log.data");
		mFilesMap["block_processing"] = data_logger::fileData("log/dr-monero/block_proc.data");
		
		mFilesMap["peers_limit"] = data_logger::fileData("log/dr-monero/peers_limit.info");
		mFilesMap["download_limit"] = data_logger::fileData("log/dr-monero/limit_down.info");
		mFilesMap["upload_limit"] = data_logger::fileData("log/dr-monero/limit_up.info");
		
		mFilesMap["peers_limit"].mLimitFile = true;
		mFilesMap["download_limit"].mLimitFile = true;
		mFilesMap["upload_limit"].mLimitFile = true;

		// do NOT modify mFilesMap below this point, since there is no locking for this used (yet)

		_note_c("dbg/data","Creating thread for data logger"); // create timer thread
		m_thread_maybe_running=true;
		std::shared_ptr<std::thread> logger_thread(new std::thread([&]() {
			_note_c("dbg/data","Inside thread for data logger");
			while (m_state == data_logger_state::state_during_init) { // wait for creation to be done (in other thread, in singleton) before actually running
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			_note_c("dbg/data","Inside thread for data logger - going into main loop");
			while (m_state == data_logger_state::state_ready_to_use) { // run as long as we are not closing the single object
				std::this_thread::sleep_for(std::chrono::seconds(1));
				saveToFile(); // save all the pending data
			}
			_note_c("dbg/data","Inside thread for data logger - done the main loop");
			m_thread_maybe_running=false;
		}));
		logger_thread->detach();
		_info_c("dbg/data","Data logger constructed");
	}

	data_logger::~data_logger() {
		_note_c("dbg/data","Destructor of the data logger");
		{
			std::lock_guard<std::mutex> lock(mMutex);
			m_state = data_logger_state::state_dying;
		}
		_info_c("dbg/data","State was set to dying");
		while(m_thread_maybe_running) { // wait for the thread to exit
			std::this_thread::sleep_for(std::chrono::seconds(1));
			_info_c("dbg/data","Waiting for background thread to exit");
		}
		_info_c("dbg/data","Thread exited");
	}

	void data_logger::kill_instance() { 
		m_state = data_logger_state::state_dying;
		m_obj.reset();
	}
	
	void data_logger::add_data(std::string filename, unsigned int data) {
		std::lock_guard<std::mutex> lock(mMutex);
		if (m_state != data_logger_state::state_ready_to_use) { _info_c("dbg/data","Data logger is not ready, returning."); return; }

		if (mFilesMap.find(filename) == mFilesMap.end()) { // no such file/counter
			_erro_c("dbg/data","Trying to use not opened data file filename="<<filename);
			_erro_c("dbg/data","Disabling saving of graphs due to error");
			m_save_graph=false; // <--- disabling saving graphs
			return;
		}
		
		if (mFilesMap[filename].mLimitFile) { // this holds a number (that is not additive) - e.g. the limit setting
			mFilesMap[filename].mDataToSave = data;
		} else {
			mFilesMap[filename].mDataToSave += data; // this holds a number that should be sum of all accumulated samples
		}
	}
	
	bool data_logger::is_dying() {
		if (m_state == data_logger_state::state_dying) {
			return true;
		}
		else {
			return false;
		}
	}

	void data_logger::saveToFile() {
		_dbg2_c("dbg/data","saving to files");
		std::lock_guard<std::mutex> lock(mMutex);
		if (m_state != data_logger_state::state_ready_to_use) { _info_c("dbg/data","Data logger is not ready, returning."); return; }
		nOT::nUtils::cFilesystemUtils::CreateDirTree("log/dr-monero/net/");
		for (auto &element : mFilesMap)
		{
			element.second.save();
			if (!element.second.mLimitFile) element.second.mDataToSave = 0;
		}
	}

	// the inner class:
	
	double data_logger::fileData::get_current_time() {
		#if defined(__APPLE__)
		auto point = std::chrono::system_clock::now();
		#else
		auto point = std::chrono::steady_clock::now();
		#endif
		auto time_from_epoh = point.time_since_epoch();
		auto ms = std::chrono::duration_cast< std::chrono::milliseconds >( time_from_epoh ).count();
		double ms_f = ms;
		return ms_f / 1000.;
	}
	
	data_logger::fileData::fileData(std::string pFile) {
		_dbg3_c("dbg/data","opening data file named pFile="<<pFile<<" for this="<<this);
		mFile = std::make_shared<std::ofstream> (pFile);
		_dbg1_c("dbg/data","opened data file named pFile="<<pFile<<" in mFile="<<mFile<<" for this="<<this);
		mPath = pFile;
	}
	
	void data_logger::fileData::save() {
		if (!data_logger::m_save_graph) return; // <--- disabled
		_dbg2_c("dbg/data","saving to the file now, mFile="<<mFile);
		mFile->open(mPath, std::ios::app);
		*mFile << static_cast<int>(get_current_time()) << " " << mDataToSave << std::endl;
		mFile->close();
	}
	
	
data_logger_state data_logger::m_state(data_logger_state::state_before_init); ///< (static) state of the singleton object
std::atomic<bool> data_logger::m_save_graph(false); // (static)
std::atomic<bool> data_logger::m_thread_maybe_running(false); // (static)
std::once_flag data_logger::m_singleton; // (static)
std::unique_ptr<data_logger> data_logger::m_obj; // (static)

} // namespace
} // namespace

