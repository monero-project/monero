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

enum class data_logger_state { state_before_init, state_during_init, state_ready_to_use, state_dying };
	
/***
@note: use it ONLY via singleton! It will be spawned then, and will auto destruct on program exit.
@note: do call ::kill_instance() before exiting main, at end of main. But before make sure no one else (e.g. no other threads) will try to use this/singleton
@note: it is not allowed to use this class from code "runnig before or after main", e.g. from ctors of static objects, because of static-creation-order races
@note: on creation (e.g. from singleton), it spawns a thread that saves all data in background
*/
	class data_logger {
		public:
			static data_logger &get_instance(); ///< singleton
			static void kill_instance(); ///< call this before ending main to allow more gracefull shutdown of the main singleton and it's background thread
			~data_logger(); ///< destr, will be called when singleton is killed when global m_obj dies. will kill theads etc

		private:
			data_logger(); ///< constructor is private, use only via singleton get_instance

		public:
			data_logger(const data_logger &ob) = delete; // use only one per program
			data_logger(data_logger &&ob) = delete;
			data_logger & operator=(const data_logger&) = delete;
			data_logger & operator=(data_logger&&) = delete;

			void add_data(std::string filename, unsigned int data); ///< use this to append data here. Use it only the singleton. It locks itself.

			static std::atomic<bool> m_save_graph; ///< global setting flag, should we save all the data or not (can disable logging graphs data)

		private:
			static std::once_flag m_singleton; ///< to guarantee singleton creates the object exactly once
			static data_logger_state m_state; ///< state of the singleton object
			static std::atomic<bool> m_thread_maybe_running; ///< is the background thread (more or less) running, or is it fully finished
			static std::unique_ptr<data_logger> m_obj; ///< the singleton object. Only use it via get_instance(). Can be killed by kill_instance()

			/***
			* one graph/file with data
			*/
			class fileData {
				public:
					fileData() = default;
					fileData(const fileData &ob) = delete;
					fileData(std::string pFile);
					
					std::shared_ptr<std::ofstream> mFile;
					long int mDataToSave = 0; ///< sum of the data (in current interval, will be counted from 0 on next interval)
					static double get_current_time();
					void save();
					std::string mPath;
					bool mLimitFile = false; ///< this holds a number (that is not additive) - e.g. the limit setting
			};
			
			std::map<std::string, fileData> mFilesMap;
			std::mutex mMutex;
			void saveToFile(); ///< write data to the target files. do not use this directly
	};
	
} // namespace
} // namespace

#endif
