PROG = nc
SOURCES = $(PROG).c ../../net_skeleton.c
CFLAGS = -W -Wall -I../.. -pthread -DNS_ENABLE_SSL -lssl $(CFLAGS_EXTRA)

all: $(PROG)

$(PROG): $(SOURCES)
	$(CC) $(SOURCES) -o $@ $(CFLAGS)

$(PROG).exe: $(SOURCES)
	cl $(SOURCES) /I../.. /DNS_ENABLE_SSL /MD /Fe$@

clean:
	rm -rf *.gc* *.dSYM *.exe *.obj *.o a.out $(PROG)
