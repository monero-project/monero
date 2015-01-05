/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief various general utils taken from (and relate to) otshell project, including loggiang/debug

/* See other files here for the LICENCE that applies here. */

#include "ccolor.hpp"
#ifndef INCLUDE_OT_NEWCLI_UTILS
#define INCLUDE_OT_NEWCLI_UTILS

#include "lib_common1.hpp"
#ifdef __unix
	#include <unistd.h>
#endif

#ifndef CFG_WITH_TERMCOLORS
	#error "You requested to turn off terminal colors (CFG_WITH_TERMCOLORS), however currently they are hardcoded (this option to turn them off is not yet implemented)."
#endif

///Macros related to automatic deduction of class name etc; 
#define MAKE_CLASS_NAME(NAME) private: static std::string GetObjectName() { return #NAME; }
#define MAKE_STRUCT_NAME(NAME) private: static std::string GetObjectName() { return #NAME; } public:

namespace nOT {

namespace nUtils {

/// @brief general based for my runtime errors
class myexception : public std::runtime_error {
	public:
		myexception(const char * what);
		myexception(const std::string &what);
        //virtual ~myexception();
		virtual void Report() const;
};

/// @macro Use this macro INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1 as a shortcut for various using std::string etc.
INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1; // <=== namespaces

// ======================================================================================
/// text trimming functions (they do mutate the passes string); they trim based on std::isspace. also return it's reference again
/// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
std::string & trim(std::string &s); ///< trim text http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
std::string & ltrim(std::string &s); ///< left trim
std::string & rtrim(std::string &s); ///< right trim

// ======================================================================================

std::string get_current_time();

// string conversions
template <class T>
std::string ToStr(const T & obj) {
	std::ostringstream oss;
	oss << obj;
	return oss.str();
}

struct cNullstream : std::ostream {
	cNullstream() : std::ios(0), std::ostream(0) {}
};
extern cNullstream g_nullstream; // a stream that does nothing (eats/discards data)
// ========== debug ==========
// _dbg_ignore is moved to global namespace (on purpose)

// TODO make _dbg_ignore thread-safe everywhere

extern std::mutex gLoggerGuard;



#define _debug_level_c(CHANNEL,LEVEL,VAR) do { if (_dbg_ignore< LEVEL) { \
	nOT::nUtils::gLoggerGuard.try_lock(); \
	gCurrentLogger.write_stream(LEVEL,CHANNEL) << nOT::nUtils::get_current_time() << ' ' << OT_CODE_STAMP << ' ' << VAR << gCurrentLogger.endline() << std::flush; \
	nOT::nUtils::gLoggerGuard.unlock(); \
	} } while(0)

#define _debug_level(LEVEL,VAR) _debug_level_c("",LEVEL,VAR)

#define _dbg3(VAR) _debug_level( 20,VAR)
#define _dbg2(VAR) _debug_level( 30,VAR)
#define _dbg1(VAR) _debug_level( 40,VAR) // details
#define _info(VAR) _debug_level( 50,VAR) // more boring info
#define _note(VAR) _debug_level( 70,VAR) // info
#define _fact(VAR) _debug_level( 75,VAR) // interesting event
#define _mark(VAR) _debug_level( 80,VAR) // marked action
#define _warn(VAR) _debug_level( 90,VAR) // some problem
#define _erro(VAR) _debug_level(100,VAR) // error - report

#define _dbg3_c(C,VAR) _debug_level_c(C, 20,VAR)
#define _dbg2_c(C,VAR) _debug_level_c(C, 30,VAR)
#define _dbg1_c(C,VAR) _debug_level_c(C, 40,VAR) // details
#define _info_c(C,VAR) _debug_level_c(C, 50,VAR) // more boring info
#define _note_c(C,VAR) _debug_level_c(C, 70,VAR) // info
#define _fact_c(C,VAR) _debug_level_c(C, 75,VAR) // interesting event
#define _mark_c(C,VAR) _debug_level_c(C, 80,VAR) // marked action
#define _warn_c(C,VAR) _debug_level_c(C, 90,VAR) // some problem
#define _erro_c(C,VAR) _debug_level_c(C,100,VAR) // error - report

// lock // because od VAR
#define _scope_debug_level_c(CHANNEL,LEVEL,VAR) \
	std::ostringstream debug_detail_oss; \
	nOT::nUtils::gLoggerGuard.try_lock(); \
	debug_detail_oss << OT_CODE_STAMP << ' ' << VAR ; \
	nOT::nUtils::nDetail::cDebugScopeGuard debugScopeGuard; \
	if (_dbg_ignore<LEVEL) debugScopeGuard.Assign(CHANNEL,LEVEL, debug_detail_oss.str()); \
	if (_dbg_ignore<LEVEL) _debug_level_c(CHANNEL,LEVEL,debug_detail_oss.str() + " ... begin"); \
	nOT::nUtils::gLoggerGuard.unlock();
#define _scope_debug_level(LEVEL,VAR) _scope_debug_level_c("",LEVEL,VAR)

#define _scope_dbg1(VAR) _scope_debug_level( 20,VAR)
#define _scope_dbg2(VAR) _scope_debug_level( 30,VAR)
#define _scope_dbg3(VAR) _scope_debug_level( 40,VAR) // details
#define _scope_info(VAR) _scope_debug_level( 50,VAR) // more boring info
#define _scope_note(VAR) _scope_debug_level( 70,VAR) // info
#define _scope_fact(VAR) _scope_debug_level( 75,VAR) // interesting event
#define _scope_mark(VAR) _scope_debug_level( 80,VAR) // marked action
#define _scope_warn(VAR) _scope_debug_level( 90,VAR) // some problem
#define _scope_erro(VAR) _scope_debug_level( 100,VAR) // error - report

/***
@brief do not use this namespace directly, it is implementation detail.
*/
namespace nDetail { 

/***
@brief a Debug scope-guard, to log a debug message when current scope is left. Do NOT use this directly,
only use it via the macros like _scope_dbg1 etc.
*/
class cDebugScopeGuard {
	protected:
		string mMsg;
		int mLevel;
		string mChan;
	public:
		cDebugScopeGuard();
		~cDebugScopeGuard();
		void Assign(const string &chan, const int level, const string &msg);
};

const char* DbgShortenCodeFileName(const char *s); ///< Returns a pointer to some part of the string that was given, skipping directory names, for log/debug

}; // namespace nDetail

// ========== logger ==========

/*** 
@brief Class to write debug into. Used it by calling the debug macros _dbg1(...) _info(...) _erro(...) etc, NOT directly!
@author rfree (maintainer) 
*/
class cLogger {
	public:
		cLogger();
		~cLogger();
		std::ostream & write_stream(int level); ///< starts a new message on given level (e.g. writes out the icon/tag) and returns stream to output to
		std::ostream & write_stream(int level, const std::string & channel); ///< the same but with name of the debug channel

		void setOutStreamFromGlobalOptions(); // set debug level, file etc - according to global Options
		void setOutStreamFile(const std::string &fname); // switch to using this file
		void setDebugLevel(int level); // change the debug level e.g. to mute debug from now

		std::string icon(int level) const; ///< returns "icon" for given debug level. It is text, might include color controll characters
		std::string endline() const; ///< returns string to be written at end of message

	protected:
		unique_ptr<std::ofstream> mOutfile;
		std::ostream * mStream; ///< pointing only! can point to our own mOutfile, or maye to global null stream

		std::map< std::string , std::ofstream * > mChannels; // the ofstream objects are owned by this class

		int mLevel; ///< current debug level

		std::ostream & SelectOutput(int level, const std::string & channel);
		void OpenNewChannel(const std::string & channel);
		std::string GetLogBaseDir() const;

		std::map< std::thread::id , int > mThread2Number; // change long thread IDs into a short nice number to show
		int mThread2Number_Biggest; // current biggest value held there (biggest key) - works as growing-only counter basically
		int Thread2Number(const std::thread::id id); // convert the system's thread id into a nice short our id; make one if new thread
};



// ====================================================================
// vector debug

template <class T>
std::string vectorToStr(const T & v) {
	std::ostringstream oss;
	for(auto rec: v) {
		oss << rec <<",";
		}
	return oss.str();
}

template <class T>
void DisplayVector(std::ostream & out, const std::vector<T> &v, const std::string &delim=" ") {
	std::copy( v.begin(), v.end(), std::ostream_iterator<T>(out, delim.c_str()) );
}

template <class T>
void EndlDisplayVector(std::ostream & out, const std::vector<T> &v, const std::string &delim=" ") {
	out << std::endl;
	DisplayVector(out,v,delim);
}

template <class T>
void DisplayVectorEndl(std::ostream & out, const std::vector<T> &v, const std::string &delim=" ") {
	DisplayVector(out,v,delim);
	out << std::endl;
}

template <class T>
void DbgDisplayVector(const std::vector<T> &v, const std::string &delim=" ") {
	std::cerr << "[";
	std::copy( v.begin(), v.end(), std::ostream_iterator<T>(std::cerr, delim.c_str()) );
	std::cerr << "]";
}

string stringToColor(const string &hash);
template <class T, class T2>
void DisplayMap(std::ostream & out, const std::map<T, T2> &m, const std::string &delim=" ") {
	auto *no_color = zkr::cc::fore::console;
	for(auto var : m) {
		out << stringToColor(var.first) << var.first << delim << var.second << no_color << endl;
	}

}

template <class T, class T2>
void EndlDisplayMap(std::ostream & out, const std::map<T, T2> &m, const std::string &delim=" ") {
	out << endl;
	for(auto var : m) {
		out << var.first << delim << var.second << endl;
	}
}

template <class T, class T2>
void DbgDisplayMap(const std::map<T, T2> &m, const std::string &delim=" ") {
	for(auto var : m) {
		std::cerr << var.first << delim << var.second << endl;
	}
}


template <class T>
void DbgDisplayVectorEndl(const std::vector<T> &v, const std::string &delim=" ") {
	DbgDisplayVector(v,delim);
	std::cerr << std::endl;
}

void DisplayStringEndl(std::ostream & out, const std::string text);

bool CheckIfBegins(const std::string & beggining, const std::string & all);
bool CheckIfEnds (std::string const & ending, std::string const & all);
std::string SpaceFromEscape(const std::string &s);
std::string EscapeFromSpace(const std::string &s);
vector<string> WordsThatMatch(const std::string & sofar, const vector<string> & possib);
char GetLastChar(const std::string & str);
std::string GetLastCharIf(const std::string & str); // TODO unicode?
std::string EscapeString(const std::string &s);


template <class T>
std::string DbgVector(const std::vector<T> &v, const std::string &delim="|") {
	std::ostringstream oss;
	oss << "[";
	bool first=true;
	for(auto vElement : v) { if (!first) oss<<delim; first=false; oss <<vElement ; }
	oss << "]";
	//std::copy( v.begin(), v.end(), std::ostream_iterator<T>(oss, delim.c_str()) );
	return oss.str();
}

template <class T>
std::ostream & operator<<(std::ostream & os, const map< T, vector<T> > & obj){
	os << "[";
	for(auto const & elem : obj) {
		os << " [" << elem.first << "=" << DbgVector(elem.second) << "] ";
	}
	os << "]";
  return os;
}

template <class T, class T2>
std::string DbgMap(const map<T, T2> & map) {
	std::ostringstream oss;
		oss << map;
	return oss.str();
}

// ====================================================================
// assert

// ASRT - assert. Name like ASSERT() was too long, and ASS() was just... no.
// Use it like this: ASRT( x>y );  with the semicolon at end, a clever trick forces this syntax :)
#define ASRT(x) do { if (!(x)) nOT::nUtils::Assert(false, OT_CODE_STAMP, #x); } while(0)

void Assert(bool result, const std::string &stamp, const std::string &condition);

// ====================================================================
// advanced string

const std::string GetMultiline(string endLine = "~");
vector<string> SplitString(const string & str);

bool checkPrefix(const string & str, char prefix = '^');

// ====================================================================
// nUse utils

enum class eSubjectType {Account, Asset, User, Server, Unknown};

string SubjectType2String(const eSubjectType & type);
eSubjectType String2SubjectType(const string & type);

// ====================================================================
// operation on files

/// @brief tools related to filesystem
/// @author rfree (maintainer)
class cFilesystemUtils { // if we do not want to use boost in given project (or we could optionally write boost here later)
	public:
		static bool CreateDirTree(const std::string & dir, bool only_below=false);
		static char GetDirSeparator(); // eg '/' or '\'
};


/// @brief utils to e.g. edit a file from console
/// @author rfree (maintainer)
class cEnvUtils {
	int fd;
	string mFilename;

	void GetTmpTextFile();
	void CloseFile();
	void OpenEditor();
	const string ReadFromTmpFile();
public:
	const string Compose();
	const string ReadFromFile(const string path);
};
void hintingToTxt(std::fstream & file, string command, vector<string> &commands);
void generateQuestions (std::fstream & file, string command);
void generateAnswers (std::fstream & file, string command, vector<string> &completions);

// ====================================================================

namespace nOper { // nOT::nUtils::nOper
// cool shortcut operators, like vector + vecotr operator working same as string (appending)
// isolated to namespace because it's unorthodox ide to implement this

using namespace std;

// TODO use && and move?
template <class T>
vector<T> operator+(const vector<T> &a, const vector<T> &b) {
	vector<T> ret = a;
	ret.insert( ret.end() , b.begin(), b.end() );
	return ret;
}

template <class T>
vector<T> operator+(const T &a, const vector<T> &b) {
	vector<T> ret(1,a);
	ret.insert( ret.end() , b.begin(), b.end() );
	return ret;
}

template <class T>
vector<T> operator+(const vector<T> &a, const T &b) {
	vector<T> b_vector(1,a);
	return a + b_vector;
}

template <class T>
vector<T>& operator+=(vector<T> &a, const vector<T> &b) {
	a.insert( a.end() , b.begin(), b.end() );
	return a;
}

// map
template <class TK,class TV>
map<TK,TV> operator+(const map<TK,TV> &a, const map<TK,TV> &b) {
	map<TK,TV> ret = a;
	for (const auto & elem : b) {
		ret.insert(elem);
	}
	return ret;
}


} // nOT::nUtils::nOper

// ====================================================================

// ====================================================================

// Algorithms

// ====================================================================
// ====================================================================


/**
@brief Special type that on creation will be initialized to have value INIT given as template argument. 
Might be usefull e.g. to express in the declaration of class what will be the default value of member variable
See also http://www.boost.org/doc/libs/1_56_0/libs/utility/value_init.htm
Probably not needed when using boost in your project.
*/
template <class T, T INIT>
class value_init {
	private:
		T data;
	public:
		value_init();

		T& operator=(const T& v) { data=v; return *this; }
		operator T const &() const { return data; }
		operator T&() { return data;	}
};

template <class T, T INIT>
value_init<T, INIT>::value_init() :	data(INIT) { }

}; // namespace nUtils

}; // namespace nOT


// global namespace
extern nOT::nUtils::cLogger gCurrentLogger; ///< The current main logger. Usually do not use it directly, instead use macros like _dbg1 etc

std::string GetObjectName(); ///< Method to return name of current object; To use in debug; Can be shadowed in your classes. (Might be not used currently)

const extern int _dbg_ignore; ///< the global _dbg_ignore, but local code (blocks, classes etc) you could shadow it in your code blocks, 
// to override debug compile-time setting for given block/class, e.g. to disable debug in one of your methods or increase it there.
// Or to make it runtime by providing a class normal member and editing it in runtime

#define OT_CODE_STAMP ( nOT::nUtils::ToStr("[") + nOT::nUtils::nDetail::DbgShortenCodeFileName(__FILE__) + nOT::nUtils::ToStr("+") + nOT::nUtils::ToStr(__LINE__) + nOT::nUtils::ToStr(" ") + (GetObjectName()) + nOT::nUtils::ToStr("::") + nOT::nUtils::ToStr(__FUNCTION__) + nOT::nUtils::ToStr("]"))




#endif

