PROG = unit_test
PROF = -fprofile-arcs -ftest-coverage -g -O0
CFLAGS = -W -Wall -Werror -I.. -DNS_ENABLE_SSL -DNS_ENABLE_IPV6 -pthread $(PROF) $(CFLAGS_EXTRA)
PEDANTIC=$(shell gcc --version 2>/dev/null | grep -q clang && echo -pedantic)
SOURCES = $(PROG).c ../net_skeleton.c

# http://crossgcc.rts-software.org/doku.php?id=compiling_for_win32
MINGW_GCC=/usr/local/gcc-4.8.0-qt-4.8.4-for-mingw32/win32-gcc/bin/i586-mingw32-gcc

all: clean ismerged $(PROG)_ansi $(PROG)_c99 $(PROG)

.PHONY: clean clean_coverage $(PROG) $(PROG)_ansi $(PROG)_c99 $(PROG).exe $(PROG)_mingw.exe

ismerged:
	$(MAKE) -C ../modules -s --no-print-directory merge_net_skeleton.c | diff ../net_skeleton.c -
	$(MAKE) -C ../modules -s --no-print-directory merge_net_skeleton.h | diff ../net_skeleton.h -

$(PROG): Makefile
	$(MAKE) clean_coverage
	g++ -x c++ $(SOURCES) -o $(PROG) $(CFLAGS) -lssl && ./$(PROG)
	gcov -b $(PROG).c

$(PROG)_ansi: Makefile
	$(MAKE) clean_coverage
	gcc $(PEDANTIC) -ansi $(SOURCES) -o $(PROG)_ansi $(CFLAGS) -lssl && ./$(PROG)_ansi
	gcov -b $(PROG).c

$(PROG)_c99: Makefile
	$(MAKE) clean_coverage
	gcc $(PEDANTIC) -std=c99 $(SOURCES) -o $(PROG)_c99 $(CFLAGS) -lssl && ./$(PROG)_c99

$(PROG)_mingw.exe: Makefile
	$(MAKE) clean_coverage
	$(MINGW_GCC) $(SOURCES) -o $(PROG)_mingw.exe -W -Wall -Werror -I..

$(PROG).exe:
	wine cl $(SOURCES) /MD /I.. /Zi $(CFLAGS_EXTRA) && echo "Compiled OK\n"

win: $(PROG).exe
	wine $(PROG).exe

clean: clean_coverage
	rm -rf $(PROG) $(PROG)_ansi $(PROG)_c99 *.txt *.exe *.obj *.o a.out *.pdb *.opt

clean_coverage:
	rm -rf *.gc* *.dSYM
