#ifndef INCLUDED_p2p_data_logger_hpp
#define INCLUDED_p2p_data_logger_hpp

#include <string>
#include <map>
#include <fstream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

namespace epee
{
namespace net_utils
{
	
	class data_logger {
		public:
			static data_logger &get_instance();
			data_logger(const data_logger &ob) = delete;
			data_logger(data_logger &&ob) = delete;
			void add_data(std::string filename, unsigned int data);
			static std::atomic<bool> m_save_graph;
		private:
			data_logger();
			class fileData
			{
				public:
					fileData(){}
					fileData(std::string pFile);
					
					std::shared_ptr<std::ofstream> mFile;
					long int mDataToSave = 0;
					static double get_current_time();
					void save();
			};
			
			std::map <std::string, fileData> mFilesMap;
			std::mutex mSaveMutex;
			void saveToFile();
	};
	
} // namespace
} // namespace

#endif
