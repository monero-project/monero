#include "ccolor.hpp"
#include <cstdarg>

// from http://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
// from http://wiznet.gr/src/ccolor.zip
// edited by rfree - as part of https://github.com/rfree/Open-Transactions/

using namespace std;

#ifdef _MSC_VER

#define snprintf c99_snprintf

inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap) {
	int count = -1;
	if (size != 0)
		 count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
	if (count == -1)
		 count = _vscprintf(format, ap);
	return count;
}

inline int c99_snprintf(char* str, size_t size, const char* format, ...) {
	int count;
	va_list ap;
	va_start(ap, format);
	count = c99_vsnprintf(str, size, format, ap);
	va_end(ap);
	return count;
}
#endif // _MSC_VER

#define CC_CONSOLE_COLOR_DEFAULT "\033[0m"
#define CC_FORECOLOR(C) "\033[" #C "m"
#define CC_BACKCOLOR(C) "\033[" #C "m"
#define CC_ATTR(A) "\033[" #A "m"

namespace zkr
{
	enum Color
	{
		Black,
		Red,
		Green,
		Yellow,
		Blue,
		Magenta,
		Cyan,
		White,
		Default = 9
	};

	enum Attributes
	{
		Reset,
		Bright,
		Dim,
		Underline,
		Blink,
		Reverse,
		Hidden
	};

	char * cc::color(int attr, int fg, int bg)
	{
				static const int size = 20;
        static char command[size];

        /* Command is the control command to the terminal */
        snprintf(command, size, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
        return command;
	}


	const char *cc::console = CC_CONSOLE_COLOR_DEFAULT;
	const char *cc::underline = CC_ATTR(4);
	const char *cc::bold = CC_ATTR(1);

	const char *cc::fore::black = CC_FORECOLOR(30);
	const char *cc::fore::blue = CC_FORECOLOR(34);
	const char *cc::fore::red = CC_FORECOLOR(31);
	const char *cc::fore::magenta = CC_FORECOLOR(35);
	const char *cc::fore::green = CC_FORECOLOR(92);
	const char *cc::fore::cyan = CC_FORECOLOR(36);
	const char *cc::fore::yellow = CC_FORECOLOR(33);
	const char *cc::fore::white = CC_FORECOLOR(37);
	const char *cc::fore::console = CC_FORECOLOR(39);

	const char *cc::fore::lightblack = CC_FORECOLOR(90);
	const char *cc::fore::lightblue = CC_FORECOLOR(94);
	const char *cc::fore::lightred = CC_FORECOLOR(91);
	const char *cc::fore::lightmagenta = CC_FORECOLOR(95);
	const char *cc::fore::lightgreen = CC_FORECOLOR(92);
	const char *cc::fore::lightcyan = CC_FORECOLOR(96);
	const char *cc::fore::lightyellow = CC_FORECOLOR(93);
	const char *cc::fore::lightwhite = CC_FORECOLOR(97);

	const char *cc::back::black = CC_BACKCOLOR(40);
	const char *cc::back::blue = CC_BACKCOLOR(44);
	const char *cc::back::red = CC_BACKCOLOR(41);
	const char *cc::back::magenta = CC_BACKCOLOR(45);
	const char *cc::back::green = CC_BACKCOLOR(42);
	const char *cc::back::cyan = CC_BACKCOLOR(46);
	const char *cc::back::yellow = CC_BACKCOLOR(43);
	const char *cc::back::white = CC_BACKCOLOR(47);
	const char *cc::back::console = CC_BACKCOLOR(49);

	const char *cc::back::lightblack = CC_BACKCOLOR(100);
	const char *cc::back::lightblue = CC_BACKCOLOR(104);
	const char *cc::back::lightred = CC_BACKCOLOR(101);
	const char *cc::back::lightmagenta = CC_BACKCOLOR(105);
	const char *cc::back::lightgreen = CC_BACKCOLOR(102);
	const char *cc::back::lightcyan = CC_BACKCOLOR(106);
	const char *cc::back::lightyellow = CC_BACKCOLOR(103);
	const char *cc::back::lightwhite = CC_BACKCOLOR(107);
}
