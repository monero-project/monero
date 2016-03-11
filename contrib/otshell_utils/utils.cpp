/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief various general utils taken from (and relate to) otshell project, including loggiang/debug

/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>

#include "utils.hpp"

#include "ccolor.hpp"

#include "lib_common1.hpp"

#include "runoptions.hpp"

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined (WIN64)
	#define OS_TYPE_WINDOWS
#elif defined(__unix__) || defined(__posix) || defined(__linux) || defined(__darwin) || defined(__APPLE__) || defined(__clang__)
	#define OS_TYPE_POSIX
#else
	#warning "Compiler/OS platform is not recognized. Just assuming it will work as POSIX then"
	#define OS_TYPE_POSIX
#endif

#if defined(OS_TYPE_WINDOWS)
	#include <windows.h>
	#include <process.h>
#elif defined(OS_TYPE_POSIX)
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
#else
	#error "Compiler/OS platform detection failed - not supported"
#endif


namespace nOT {
namespace nUtils {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1 // <=== namespaces

// ====================================================================

// Numerical values of the debug levels - see hpp
const int _debug_level_nr_dbg3=20;
const int _debug_level_nr_dbg2=30;
const int _debug_level_nr_dbg1=40;
const int _debug_level_nr_info=50;
const int _debug_level_nr_note=60;
const int _debug_level_nr_fact=75;
const int _debug_level_nr_mark=80;
const int _debug_level_nr_warn=90;
const int _debug_level_nr_erro=100;

// ====================================================================

myexception::myexception(const char * what)
	: std::runtime_error(what)
{ }

myexception::myexception(const std::string &what)
	: std::runtime_error(what)
{ }

void myexception::Report() const {
	_erro("Error: " << what());
}

//myexception::~myexception() { }

// ====================================================================

// text trimming
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
std::string & ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

std::string & rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
  return s;
}

std::string & trim(std::string &s) {
	return ltrim(rtrim(s));
}

std::string get_current_time() {
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  time_t time_now = std::chrono::system_clock::to_time_t(now);
  std::chrono::high_resolution_clock::duration duration = now.time_since_epoch();
  int64_t micro = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

  // std::localtime() - This function may not be thread-safe.
	#ifdef OS_TYPE_WINDOWS
		struct tm * tm_pointer = std::localtime( &time_now ); // thread-safe on mingw-w64 (thread local variable) and on MSVC btw
		// http://stackoverflow.com/questions/18551409/localtime-r-support-on-mingw
		// tm_pointer points to thread-local data, memory is owned/managed by the system/library
	#else
		// linux, freebsd, have this
		struct tm tm_object; // automatic storage duration http://en.cppreference.com/w/cpp/language/storage_duration
		struct tm * tm_pointer = & tm_object; // just point to our data
		auto x = localtime_r( &time_now , tm_pointer    ); // modifies our own (this thread) data in tm_object, this is safe http://linux.die.net/man/3/localtime_r
		if (x != tm_pointer) return "(internal error in get_current_time)"; // redundant check in case of broken implementation of localtime_r
	#endif
	// tm_pointer now points to proper time data, and that memory is automatically managed
	if (!tm_pointer) return "(internal error in get_current_time - NULL)"; // redundant check in case of broken implementation of used library methods

	std::stringstream stream;
		stream << std::setfill('0')
		<< std::setw(2) << tm_pointer->tm_year+1900
		<< '-' << std::setw(2) << tm_pointer->tm_mon+1
		<< '-' << std::setw(2) << tm_pointer->tm_mday
		<< ' ' << std::setw(2) << tm_pointer->tm_hour
		<< ':' << std::setw(2) << tm_pointer->tm_min
		<< ':' << std::setw(2) << tm_pointer->tm_sec
		<< '.' << std::setw(6) << (micro%1000000); // 6 because microseconds
  return stream.str();
}

cNullstream g_nullstream; // extern a stream that does nothing (eats/discards data)

boost::recursive_mutex gLoggerGuard; // extern
std::atomic<int> gLoggerGuardDepth; // extern

std::atomic<int> & gLoggerGuardDepth_Get() {
	// TODO std::once would be nicer here

	static bool once=0;

	if (!once) { // initialize it once
		once=1;
		gLoggerGuardDepth=0;	
	}

	return gLoggerGuardDepth; // global, atomic counter
}


// ====================================================================

namespace nDetail {

const char* DbgShortenCodeFileName(const char *s) {
	const char *p = s;
	const char *a = s;

	bool inc=1;
	while (*p) {
		++p;
		if (inc && ('\0' != * p)) { a=p; inc=false; } // point to the current character (if valid) becasue previous one was slash
		if ((*p)=='/') { a=p; inc=true; } // point at current slash (but set inc to try to point to next character)
	}
	return a;
}

}

// a workaround for MSVC compiler; e.g. see https://bugs.webkit.org/show_bug.cgi?format=multiple&id=125795
#ifndef _MSC_VER
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
	return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}
#else
	using std::make_unique;
#endif
// ====================================================================

char cFilesystemUtils::GetDirSeparatorSys() {
	// TODO nicer os detection?
	#if defined(OS_TYPE_POSIX)
		return '/';
	#elif defined(OS_TYPE_WINDOWS)
		return '\\';
	#else
		#error "Do not know how to compile this for your platform."
	#endif
}

char cFilesystemUtils::GetDirSeparatorInter() {
	return '/';
}

string cFilesystemUtils::FileInternalToSystem(const std::string &name) {
	string ret;
	ret.resize(name.size());
	std::replace_copy(name.begin(), name.end(), ret.begin(), 
		GetDirSeparatorInter() , GetDirSeparatorSys());
	return ret;
}

string cFilesystemUtils::FileSystemToInternal(const std::string &name) {
	string ret;
	ret.reserve(name.size());
	std::replace_copy(name.begin(), name.end(), ret.begin(), 
		GetDirSeparatorSys() , GetDirSeparatorInter());
	return ret;
}

bool cFilesystemUtils::CreateDirTree(const std::string & dir, bool only_below) {
	const bool dbg=false;
	//struct stat st;
	const char dirchS = cFilesystemUtils::GetDirSeparatorSys();
	const char dirchI = cFilesystemUtils::GetDirSeparatorInter();
	std::istringstream iss(dir);
	string partI; // current par is in internal format (though it should not matter since it doesn't contain any slashes). eg "bar"
	string sofarS=""; // sofarS - the so far created dir part is in SYSTEM format. eg "foo/bar"
	if (dir.size()<1) return false; // illegal name
	// dir[0] is valid from here
	if  ( only_below  &&  ((dir[0]==dirchS) || (dir[0]==dirchI))) return false; // no jumping to top (on any os)

	while (getline(iss,partI,dirchI)) { // get new component eg "bar" into part
		if (dbg) cout << '['<<partI<<']' << endl;
		sofarS += partI;
		if (partI.size()<1) return false; // bad format?
		if ((only_below) && (partI=="..")) return false; // trying to go up

		if (dbg) cout << "test ["<<sofarS<<"]"<<endl;
		// TODO nicer os detection?
		#if defined(OS_TYPE_POSIX)
			struct stat st;
			bool exists = stat(sofarS.c_str() ,&st) == 0; // *
			if (exists) {
				if (! S_ISDIR(st.st_mode)) {
					// std::cerr << "This exists, but as a file: [" << sofar << "]" << (size_t)st.st_ino << endl;
					return false; // exists but is a file nor dir
				}
			}
		#elif defined(OS_TYPE_WINDOWS)
			DWORD dwAttrib = GetFileAttributesA(sofarS.c_str());
			bool exists = (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
		#else
			#error "Do not know how to compile this for your platform."
		#endif

		if (!exists) {
			if (dbg) cout << "mkdir ["<<sofarS<<"]"<<endl;
			#if defined(OS_TYPE_POSIX)
				bool ok = 0==  mkdir(sofarS.c_str(), 0700); // ***
			#elif defined(OS_TYPE_WINDOWS)
				bool ok = (bool) CreateDirectoryA(sofarS.c_str(), NULL); // TODO use -W() after conversion to unicode UTF16
			#else
				#error "Do not know how to compile this for your platform."
			#endif
			if (!ok) return false;
		}
		sofarS += dirchS;
	}
	return true;
}
// ====================================================================

namespace nDetail {

struct channel_use_info { ///< feedback information about using (e.g. opening) given debug channel - used internally by logging system 
/// TODO not yet used in code
/// e.g. used to write into channel net/in/all that given message was a first logged message from never-before-logged thread or PID etc
	bool m_was_interesting; ///< anything interesting happened when using the channel?
	std::vector<std::string> m_extra_msg; ///< any additional messages about this channel use
};	

cDebugScopeGuard::cDebugScopeGuard() : mLevel(-1) {
}
	
cDebugScopeGuard::~cDebugScopeGuard() {
	if (mLevel != -1) {
		gCurrentLogger.write_stream(mLevel,mChan) << mMsg << " ... end" << gCurrentLogger.endline() << std::flush;
	}
}

void cDebugScopeGuard::Assign(const string &chan, const int level, const string &msg) {
	mChan=chan;
	mLevel=level;
	mMsg=msg;
}

} // namespace nDetail

// ====================================================================

cLogger::cLogger() :
mStream(NULL),
mStreamBrokenDebug(NULL),
mIsBroken(true), // before constructor finishes
mLevel(_debug_level_nr_warn),
mThread2Number_Biggest(0), // the CURRENT biggest value (no thread yet in map)
mPid2Number_Biggest(0)
{
	mStream = & std::cout;
	mStreamBrokenDebug = & std::cerr; // the backup stream
	*mStreamBrokenDebug << "Creating the logger system" << endl;
	mIsBroken=false; // ok, constr. succeeded, so string is not broken now

	// this is here, because it could be using logging itself to log creation of first thread/PID etc
	Thread2Number( boost::this_thread::get_id() ); // convert current id to short number, useful to reserve a number so that main thread is usually called 1
	Pid2Number( getpid() ); // add this proces ID as first one
}

cLogger::~cLogger() {
	for (auto pair : mChannels) {
		std::ofstream *ptr = pair.second;
		delete ptr;
		pair.second=NULL;
	}
}

void cLogger::SetStreamBroken() {
	SetStreamBroken("(no additional details about this problem)");
}

void cLogger::SetStreamBroken(const std::string &msg) {
	_dbg_dbg("Stream is broken (msg: " << msg << ")");
	if (!mIsBroken) { // if not already marked as broken
		_dbg_dbg("(It was not broken before)");
		std::cerr << OT_CODE_STAMP << "WARNING: due to a problem in the debug/logging system itself ("<<msg<<") - we are switching back to fallback stream (e.g. cerr)" << std::endl;
		if (mStreamBrokenDebug == nullptr) {
			std::cerr << OT_CODE_STAMP << " ERROR: in addition, while reporting this problem, mStreamBrokenDebug stream is NULL: " << mStreamBrokenDebug << std::endl;
		} else {
			(*mStreamBrokenDebug) << OT_CODE_STAMP << "WARNING: due to debug stream problem ("<<msg<<") - switching back to fallback stream (e.g. cerr)" << std::endl;
		}
		mIsBroken = true;
	}
}

std::ostream & cLogger::write_stream(int level) {
	return write_stream(level,"");
}

std::ostream & cLogger::write_stream(int level, const std::string & channel ) {
	_dbg_dbg("level="<<level<<" channel="<<channel);
	if (level >= mLevel) {
		if (mStream) { // TODO now disabling mStream also disables writting to any channel
			_dbg_dbg("Selecting output...");
			ostream & output = SelectOutput(level,channel);
			_dbg_dbg("Selecting output... done, output=" << (void*)(&output));
			#if defined(OS_TYPE_WINDOWS)
			output << windows_stream(level);
			#endif
			output << icon(level) << ' ';
			boost::thread::id this_id = boost::this_thread::get_id();
			output << "{" << Thread2Number(this_id) << "}";
			auto nicePid = Pid2Number(getpid());
			if (nicePid>0) output << " {p" << nicePid << "}";
			output << ' ';
			return output; // <--- return
		} else _dbg_dbg("Not writting: No mStream");
	} else _dbg_dbg("Not writting: Too low level level="<<level<<" not >= mLevel="<<mLevel);
	return g_nullstream;
}

std::string cLogger::GetLogBaseDir() const {
	return "log";
}

void cLogger::OpenNewChannel(const std::string & channel) noexcept {
	try {
		_dbg_dbg("Openning channel for channel="<<channel);
		OpenNewChannel_(channel);
	}
	catch (const std::exception &except) {
		SetStreamBroken(OT_CODE_STAMP + " Got exception when opening debug channel: " + ToStr(except.what()));
	}
	catch (...) {
		SetStreamBroken(OT_CODE_STAMP + " Got not-standard exception when opening debug channel.");
	}
}

void cLogger::OpenNewChannel_(const std::string & channel) { // channel=="net/sleep"
	_dbg_dbg("Openning channel for channel="<<channel);
	size_t last_split = channel.find_last_of(cFilesystemUtils::GetDirSeparatorInter());

	string fname_system; // the full file name in system format

	if (last_split==string::npos) { // The channel name has no directory, eg channel=="test"
		string dir = GetLogBaseDir();
		string basefile = channel + ".log";
		string fname = dir + cFilesystemUtils::GetDirSeparatorInter() + basefile;
		fname_system = cFilesystemUtils::FileInternalToSystem(fname); // <-
	}
	else { // there is a directory eg channel=="net/sleep"
		// net/sleep 
		//    ^----- last_split	
		string dir = GetLogBaseDir() + cFilesystemUtils::GetDirSeparatorInter() + channel.substr(0, last_split);
		string basefile = channel.substr(last_split+1) + ".log";
		string fname = dir + cFilesystemUtils::GetDirSeparatorInter() + basefile;
		fname_system = cFilesystemUtils::FileInternalToSystem(fname); // <-
		bool dirok = cFilesystemUtils::CreateDirTree(dir);
		if (!dirok) { string err = "In logger failed to open directory (" + dir +") for channel (" + channel +")"; throw std::runtime_error(err); }
	}

	_dbg_dbg("Openning fname_system="<<fname_system);
	std::ofstream * thefile = new std::ofstream( fname_system.c_str() ); // file system
	*thefile << "====== Log opened: " << fname_system << " (in " << ((void*)thefile) << ") ======" << endl;
//    	cerr << "====== Log opened: " << fname_system << " (in " << ((void*)thefile) << ") ======" << endl;
  	_dbg_dbg( "====== Log opened: " << fname_system << " (in " << ((void*)thefile) << ") ======" );
	mChannels.insert( std::pair<string,std::ofstream*>(channel , thefile ) ); // <- created the channel mapping
}

std::ostream & cLogger::SelectOutput(int level, const std::string & channel) noexcept {
	try {
		if (mIsBroken) { 
			_dbg_dbg("The stream is broken mIsBroken="<<mIsBroken<<" so will return backup stream");
			return *mStreamBrokenDebug;
		}
		if (channel=="") { 
			_dbg_dbg("No channel given (channel="<<channel<<") so will return main stream");
			return *mStream;
		}

		auto obj = mChannels.find(channel);
		if (obj == mChannels.end()) { // not found - need to make new channel
			_dbg_dbg("No stream openened for channel="<<channel<<" so will create it now");
			OpenNewChannel(channel); // <- create channel
			obj = mChannels.find(channel); // find again
			if (obj == mChannels.end()) { // still not found! something is wrong
				SetStreamBroken( OT_CODE_STAMP + " WARNING: can not get stream for channel="+ToStr(channel)+" level="+ToStr(channel) );
				return *mStreamBrokenDebug;
			}
		}
		auto the_stream_ptr = obj->second;
		_dbg_dbg("Found the stream file for channel="<<channel<<" as the_stream_ptr="<<the_stream_ptr);
		ASRT(the_stream_ptr);
		return *the_stream_ptr; // <--- RETURN
	} 
	catch (std::exception &except) { 
		SetStreamBroken( OT_CODE_STAMP + " Got exception: " + ToStr(except.what()) );
		_dbg_dbg("Exception! Returning broken stream");
		return *mStreamBrokenDebug;
	}
	catch (...) {
		SetStreamBroken( OT_CODE_STAMP + " Got not-standard exception.");
		_dbg_dbg("Exception! Returning broken stream");
		return *mStreamBrokenDebug;
	}

	// dead code
}

void cLogger::setOutStreamFile(const string &fname) { // switch to using this file
	_mark("WILL SWITCH DEBUG NOW to file: " << fname);
	mOutfile =  make_unique<std::ofstream>(fname);
	mStream = & (*mOutfile);
	_mark("Started new debug, to file: " << fname);
}

void cLogger::setOutStreamFromGlobalOptions() {
	if ( gRunOptions.getDebug() ) {
		if ( gRunOptions.getDebugSendToFile() ) {
			mOutfile =  make_unique<std::ofstream> ("debuglog.txt");
			mStream = & (*mOutfile);
		}
		else if ( gRunOptions.getDebugSendToCerr() ) {
			mStream = & std::cerr;
		}
		else {
			mStream = & g_nullstream;
		}
	}
	else {
		mStream = & g_nullstream;
	}
}

void cLogger::setDebugLevel(int level) {
	bool note_before = (mLevel > level); // report the level change before or after the change? (on higher level)
	if (note_before) _note("Setting debug level to "<<level);
	mLevel = level;
	if (!note_before) _note("Setting debug level to "<<level);
}

std::string cLogger::icon(int level) const {
	// TODO replan to avoid needles converting back and forth char*, string etc

	using namespace zkr;
    #if defined(OS_TYPE_POSIX)
	if (level >= 100) return cc::back::lightred     + ToStr(cc::fore::lightyellow) + ToStr("ERROR ") + ToStr(cc::fore::lightyellow) + " " ;
	if (level >=  90) return cc::back::lightyellow  + ToStr(cc::fore::black) + ToStr("Warn  ") + ToStr(cc::fore::red)+ " " ;
	if (level >=  80) return cc::back::lightmagenta + ToStr(cc::fore::black) + ToStr("MARK  "); //+ zkr::cc::console + ToStr(cc::fore::lightmagenta)+ " ";
    if (level >=  75) return cc::back::lightyellow + ToStr(cc::fore::black) + ToStr("FACT  ") + zkr::cc::console + ToStr(cc::fore::lightyellow)+ " ";
	if (level >=  70) return cc::fore::green    + ToStr("Note  ");
	if (level >=  50) return cc::fore::cyan   + ToStr("info  ");
	if (level >=  40) return cc::fore::lightwhite    + ToStr("dbg   ");
	if (level >=  30) return cc::fore::lightblue   + ToStr("dbg   ");
	if (level >=  20) return cc::fore::blue    + ToStr("dbg   ");

    #elif defined(OS_TYPE_WINDOWS)
    if (level >= 100) return ToStr("ERROR ");
    if (level >=  90) return ToStr("Warn  ");
    if (level >=  80) return ToStr("MARK  ");
    if (level >=  75) return ToStr("FACT  ");
    if (level >=  70) return ToStr("Note  ");
    if (level >=  50) return ToStr("info  ");
    if (level >=  40) return ToStr("dbg   ");
    if (level >=  30) return ToStr("dbg   ");
    if (level >=  20) return ToStr("dbg   ");
    #endif

	return "  ";
}

std::string cLogger::endline() const {
	#if defined(OS_TYPE_POSIX)
	return ToStr("") + zkr::cc::console + ToStr("\n"); // TODO replan to avoid needles converting back and forth char*, string etc
    #elif defined(OS_TYPE_WINDOWS)
    return ToStr("\n");
    #endif
}

int cLogger::Thread2Number(const boost::thread::id id) {
	auto found = mThread2Number.find( id );
	if (found == mThread2Number.end()) { // new one
		mThread2Number_Biggest++;
		mThread2Number[id] = mThread2Number_Biggest;
		_info_c("dbg/main", "This is a new thread (used in debug), thread id="<<id); // can cause some recursion
		return mThread2Number_Biggest;
	} else {
		return mThread2Number[id];
	}
}

int cLogger::Pid2Number(const t_anypid id) {
	auto found = mPid2Number.find( id );
	if (found == mPid2Number.end()) { // new one
		mPid2Number_Biggest++;
		mPid2Number[id] = mPid2Number_Biggest;
		_info_c("dbg/main", "This is a new process (used in debug), process pid="<<id); // can cause some recursion
		return mPid2Number_Biggest;
	} else {
		return mPid2Number[id];
	}
}

// ====================================================================
// object gCurrentLogger is defined later - in global namespace below


// ====================================================================
// vector debug

void DisplayStringEndl(std::ostream & out, const std::string text) {
	out << text;
	out << std::endl;
}

std::string SpaceFromEscape(const std::string &s) {
	std::ostringstream  newStr;
		for(size_t i = 0; i < s.length();i++) {
			if(s[i] == '\\' && s[i+1] ==32)
				newStr<<"";
			else
				newStr<<s[i];
			}
	return newStr.str();
}

std::string EscapeFromSpace(const std::string &s) {
	std::ostringstream  newStr;
	for(size_t i = 0; i < s.length();i++) {
		if(s[i] == 32)
			newStr << "\\" << " ";
		else
			newStr << s[i];
	}
	return newStr.str();
}


std::string EscapeString(const std::string &s) {
	std::ostringstream  newStr;
		for(size_t i = 0; i < s.length();i++) {
			if(s[i] >=32 && s[i] <= 126)
				newStr<<s[i];
			else
				newStr<<"\\"<< (int) s[i];
			}

	return newStr.str();
}


bool CheckIfBegins(const std::string & beggining, const std::string & all) {
	if (all.compare(0, beggining.length(), beggining) == 0) {
		return 1;
	}
	else {
		return 0;
	}
}

bool CheckIfEnds (std::string const & ending, std::string const & all){
	if (all.length() >= ending.length()) {
		return (0 == all.compare (all.length() - ending.length(), ending.length(), ending));
	} else {
		return false;
	}
}


vector<string> WordsThatMatch(const std::string & sofar, const vector<string> & possib) {
	vector<string> ret;
	for ( auto rec : possib) { // check of possibilities
		if (CheckIfBegins(sofar,rec)) {
			rec = EscapeFromSpace(rec);
			ret.push_back(rec); // this record matches
		}
	}
	return ret;
}

char GetLastChar(const std::string & str) { // TODO unicode?
	auto s = str.length();
	if (s==0) throw std::runtime_error("Getting last character of empty string (" + ToStr(s) + ")" + OT_CODE_STAMP);
	return str.at( s - 1);
}

std::string GetLastCharIf(const std::string & str) { // TODO unicode?
	auto s = str.length();
	if (s==0) return ""; // empty string signalizes ther is nothing to be returned
	return std::string( 1 , str.at( s - 1) );
}

// ====================================================================

// ASRT - assert. Name like ASSERT() was too long, and ASS() was just... no.
// Use it like this: ASRT( x>y );  with the semicolon at end, a clever trick forces this syntax :)

void Assert(bool result, const std::string &stamp, const std::string &condition) {
	if (!result) {
		_erro("Assert failed at "+stamp+": ASSERT( " << condition << ")");
		throw std::runtime_error("Assert failed at "+stamp+": ASSERT( " + condition + ")");
	}
}

// ====================================================================
// advanced string

const std::string GetMultiline(string endLine) {
	std::string result(""); // Taken from OT_CLI_ReadUntilEOF
	while (true) {
		std::string input_line("");
		if (std::getline(std::cin, input_line, '\n'))
		{
			input_line += "\n";
				if (input_line[0] == '~')
					break;
			result += input_line;
		}
		if (std::cin.eof() )
		{
			std::cin.clear();
				break;
		}
		if (std::cin.fail() )
		{
			std::cin.clear();
				break;
		}
		if (std::cin.bad())
		{
			std::cin.clear();
				break;
		}
	}
	return result;
}

vector<string> SplitString(const string & str){
		std::istringstream iss(str);
		vector<string> vec { std::istream_iterator<string>{iss}, std::istream_iterator<string>{} };
		return vec;
}

bool checkPrefix(const string & str, char prefix) {
	if (str.at(0) == prefix)
		return true;
	return false;
}

// ====================================================================
// operation on files


#ifdef __unix

void cEnvUtils::GetTmpTextFile() {
	// TODO make this name configurable (depending on project)
	char filename[] = "/tmp/otshellutils_text.XXXXXX";
	fd = mkstemp(filename);
	if (fd == -1) {
		_erro("Can't create the file: " << filename);
		return;
	}
	mFilename = filename;
}

void cEnvUtils::CloseFile() {
	close(fd);
	unlink( mFilename.c_str() );
}

void  cEnvUtils::OpenEditor() {
	char* editor = std::getenv("OT_EDITOR"); //TODO Read editor from configuration file
	if (editor == NULL)
		editor = std::getenv("VISUAL");
	if (editor == NULL)
		editor = std::getenv("EDITOR");

	string command;
	if (editor != NULL)
		command = ToStr(editor) + " " + mFilename;
	else
		command = "/usr/bin/editor " + mFilename;
	_dbg3("Opening editor with command: " << command);
	if ( system( command.c_str() ) == -1 )
		_erro("Cannot execute system command: " << command);
}

const string cEnvUtils::ReadFromTmpFile() {
	std::ifstream ifs(mFilename);
	string msg((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	return msg;
}

const string cEnvUtils::Compose() {
	GetTmpTextFile();
	OpenEditor();
	string input = ReadFromTmpFile();
	CloseFile();
	return input;
}

#endif

const string cEnvUtils::ReadFromFile(const string path) {
	std::ifstream ifs(path);
	string msg((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	return msg;
}

void hintingToTxt(std::fstream & file, string command, vector<string> &commands) {
	if(file.good()) {
		file<<command<<"~"<<endl;
		for (auto a: commands) {
			file <<a<< " ";
			file.flush();
		}
		file<<endl;
	}
}

string stringToColor(const string &hash) {
  // Generete vector with all possible light colors
  vector <string> lightColors;
  using namespace zkr;
  lightColors.push_back(cc::fore::lightblue);
  lightColors.push_back(cc::fore::lightred);
  lightColors.push_back(cc::fore::lightmagenta);
  lightColors.push_back(cc::fore::lightgreen);
  lightColors.push_back(cc::fore::lightcyan);
  lightColors.push_back(cc::fore::lightyellow);
  lightColors.push_back(cc::fore::lightwhite);

  int sum=0;

  for (auto ch : hash) sum+=ch;
  auto color = sum%(lightColors.size()-1);

  return lightColors.at( color );
}


// ====================================================================
// algorthms


} // namespace nUtil


} // namespace OT

// global namespace

const extern int _dbg_ignore = 0; // see description in .hpp

std::string GetObjectName() {
	//static std::string * name=nullptr;
	//if (!name) name = new std::string("(global)");
	return "";
}

// ====================================================================

nOT::nUtils::cLogger gCurrentLogger;

