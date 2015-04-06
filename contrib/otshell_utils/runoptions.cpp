/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "runoptions.hpp"

#include "lib_common1.hpp"

namespace nOT {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1 // <=== namespaces

// (no debug - this is the default)
// +nodebug (no debug)
// +debug ...... --asdf
// +debug +debugcerr .... --asfs
// +debug +debugfile .... --asfs

cRunOptions::cRunOptions()
	: mRunMode(eRunModeCurrent), mDebug(false), mDebugSendToFile(false), mDebugSendToCerr(false)
	,mDoRunDebugshow(false)
{ }

vector<string> cRunOptions::ExecuteRunoptionsAndRemoveThem(const vector<string> & args) {
	vector<string> arg_clear; // will store only the arguments that are not removed

	for (auto arg : args) {
		bool thisIsRunoption=false;

		if (arg.size()>0) {
			if (arg.at(0) == '+') thisIsRunoption=true;
		}

		if (thisIsRunoption) Exec(arg); // ***
		if (! thisIsRunoption) arg_clear.push_back(arg);
	}

	Normalize();

	return arg_clear;
}

void cRunOptions::Exec(const string & runoption) { // eg: Exec("+debug");
	if (runoption == "+nodebug") { mDebug=false; }
	else if (runoption == "+debug") { mDebug=true; }
	else if (runoption == "+debugcerr") { mDebug=true;  mDebugSendToCerr=true; }
	else if (runoption == "+debugfile") { mDebug=true;  mDebugSendToFile=true; }
	else if (runoption == "+demo") { mRunMode=eRunModeDemo; }
	else if (runoption == "+normal") { mRunMode=eRunModeNormal; }
	else if (runoption == "+current") { mRunMode=eRunModeCurrent; }
	else if (runoption == "+debugshow") { mDebug=true;  mDebugSendToCerr=true;  mDoRunDebugshow=true; }
	else {
		cerr << "Unknown runoption in Exec: '" << runoption << "'" << endl;
		throw std::runtime_error("Unknown runoption");
	}
	// cerr<<"debug="<<mDebug<<endl;
}

void cRunOptions::Normalize() {
	if (mDebug) {
		if (!(  mDebugSendToFile || mDebugSendToCerr  ))  mDebugSendToCerr=true; // if debug is on then send to something, e.g. to cerr
	}
}


cRunOptions gRunOptions; // (extern)

} // namespace OT


