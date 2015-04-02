// ccolor.hpp

// from http://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
// from http://wiznet.gr/src/ccolor.zip
// edited by rfree - as part of https://github.com/rfree/Open-Transactions/

#ifndef INCLUDE_OT_ccolor
#define INCLUDE_OT_ccolor

#include <iostream>
#include <stdio.h>

namespace zkr
{
	class cc
	{
	public:

		class fore
		{
		public:
			static const char *black;
			static const char *blue;
			static const char *red;
			static const char *magenta;
			static const char *green;
			static const char *cyan;
			static const char *yellow;
			static const char *white;
			static const char *console;

			static const char *lightblack;
			static const char *lightblue;
			static const char *lightred;
			static const char *lightmagenta;
			static const char *lightgreen;
			static const char *lightcyan;
			static const char *lightyellow;
			static const char *lightwhite;
		};

		class back
		{
		public:
			static const char *black;
			static const char *blue;
			static const char *red;
			static const char *magenta;
			static const char *green;
			static const char *cyan;
			static const char *yellow;
			static const char *white;
			static const char *console;

			static const char *lightblack;
			static const char *lightblue;
			static const char *lightred;
			static const char *lightmagenta;
			static const char *lightgreen;
			static const char *lightcyan;
			static const char *lightyellow;
			static const char *lightwhite;
		};

		static char *color(int attr, int fg, int bg);
		static const char *console;
		static const char *underline;
		static const char *bold;
	};
}

#endif

