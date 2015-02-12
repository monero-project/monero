/* See other files here for the LICENCE that applies here. */
/*
Template for new files, replace word "template" and later delete this line here.
*/

#ifndef INCLUDE_OT_NEWCLI_runoptions_hpp
#define INCLUDE_OT_NEWCLI_runoptions_hpp

#include "lib_common1.hpp"

namespace nOT {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1 // <=== namespaces

/** Global options to run this program main() Eg used for developer's special options like +setdemo +setdebug.
This is NOT for all the other options that are parsed and executed by program. */
class cRunOptions {
	public:
		enum tRunMode { ///< Type of run mode - is this normal, or demonstration etc.
			eRunModeCurrent=1, ///< currently developed version
			eRunModeDemo, ///< best currently available Demo of something nice
			eRunModeNormal, ///< do the normal things that the program should do
		};

	private:
		tRunMode mRunMode; ///< selected run mode

		bool mDebug; // turn debug on, Eg: +debug without it probably nothing will be written to debug (maybe just error etc)
		bool mDebugSendToFile; // send to file, Eg: for +debugfile ; also turns on debug
		bool mDebugSendToCerr; // send to cerr, Eg: for +debugcerr ; also turns on debug
		// if debug is set but not any other DebugSend* then we will default to sending to debugcerr

		bool mDoRunDebugshow;

	public:
		tRunMode getTRunMode() const { return mRunMode; }
		bool getDebug() const { return mDebug; }
		bool getDebugSendToFile() const { return mDebugSendToFile; }
		bool getDebugSendToCerr() const { return mDebugSendToCerr; }
		bool getDoRunDebugshow() const { return mDoRunDebugshow; }

		cRunOptions();

		vector<string> ExecuteRunoptionsAndRemoveThem(const vector<string> & args);
		void Exec(const string & runoption); // eg: Exec("+debug");

		void Normalize();
};

extern cRunOptions gRunOptions;


} // namespace nOT



#endif

