FR = ../../frozen
HEADERS = skeleton.h $(FR)/frozen.h sha1.h util.h http.h json-rpc.h
SOURCES = skeleton.c $(FR)/frozen.c http.c sha1.c util.c json-rpc.c

all: ../net_skeleton.c ../net_skeleton.h

../net_skeleton.h: Makefile $(HEADERS)
	$(MAKE) -s --no-print-directory merge_net_skeleton.h >$@

../net_skeleton.c: Makefile $(SOURCES)
	$(MAKE) -s --no-print-directory merge_net_skeleton.c >$@

merge_net_skeleton.h: Makefile $(HEADERS)
	@cat $(HEADERS)

merge_net_skeleton.c: Makefile $(SOURCES)
	@(echo '#include "net_skeleton.h"'; (cat $(SOURCES) | sed '/^#include "/d'))
