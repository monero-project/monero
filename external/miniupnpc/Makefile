# $Id: Makefile,v 1.107 2014/01/31 14:19:12 nanard Exp $
# MiniUPnP Project
# http://miniupnp.free.fr/
# http://miniupnp.tuxfamily.org/
# https://github.com/miniupnp/miniupnp
# (c) 2005-2014 Thomas Bernard
# to install use :
# $ make DESTDIR=/tmp/dummylocation install
# or
# $ INSTALLPREFIX=/usr/local make install
# or
# $ make install (default INSTALLPREFIX is /usr)
OS = $(shell uname -s)
VERSION = $(shell cat VERSION)

ifeq ($(OS), Darwin)
JARSUFFIX=mac
endif
ifeq ($(OS), Linux)
JARSUFFIX=linux
endif
ifneq (,$(findstring NT-5.1,$(OS)))
JARSUFFIX=win32
endif

HAVE_IPV6 ?= yes
export HAVE_IPV6

CC ?= gcc
#AR = gar
#CFLAGS = -O -g -DDEBUG
CFLAGS ?= -O
CFLAGS += -Wall
CFLAGS += -W -Wstrict-prototypes
CFLAGS += -fno-common
CFLAGS += -DMINIUPNPC_SET_SOCKET_TIMEOUT
CFLAGS += -DMINIUPNPC_GET_SRC_ADDR
CFLAGS += -D_BSD_SOURCE -D_POSIX_C_SOURCE=1
CFLAGS += -ansi
# -DNO_GETADDRINFO
INSTALL = install
SH = /bin/sh
JAVA = java
# see http://code.google.com/p/jnaerator/
#JNAERATOR = jnaerator-0.9.7.jar
#JNAERATOR = jnaerator-0.9.8-shaded.jar
#JNAERATORARGS = -library miniupnpc
#JNAERATOR = jnaerator-0.10-shaded.jar
JNAERATOR = jnaerator-0.11-shaded.jar
JNAERATORARGS = -mode StandaloneJar -runtime JNAerator -library miniupnpc
JNAERATORBASEURL = http://jnaerator.googlecode.com/files/

ifeq (SunOS, $(OS))
  LDFLAGS=-lsocket -lnsl -lresolv
endif

# APIVERSION is used to build SONAME
APIVERSION = 10

SRCS = igd_desc_parse.c miniupnpc.c minixml.c minisoap.c miniwget.c \
       upnpc.c upnpcommands.c upnpreplyparse.c testminixml.c \
	   minixmlvalid.c testupnpreplyparse.c minissdpc.c \
	   upnperrors.c testigddescparse.c testminiwget.c \
       connecthostport.c portlistingparse.c receivedata.c

LIBOBJS = miniwget.o minixml.o igd_desc_parse.o minisoap.o \
          miniupnpc.o upnpreplyparse.o upnpcommands.o upnperrors.o \
          connecthostport.o portlistingparse.o receivedata.o

ifneq ($(OS), AmigaOS)
CFLAGS := -fPIC $(CFLAGS)
LIBOBJS := $(LIBOBJS) minissdpc.o
endif

OBJS = $(patsubst %.c,%.o,$(SRCS))

# HEADERS to install
HEADERS = miniupnpc.h miniwget.h upnpcommands.h igd_desc_parse.h \
          upnpreplyparse.h upnperrors.h miniupnpctypes.h \
          portlistingparse.h \
          declspec.h

# library names
LIBRARY = libminiupnpc.a
ifeq ($(OS), Darwin)
  SHAREDLIBRARY = libminiupnpc.dylib
  SONAME = $(basename $(SHAREDLIBRARY)).$(APIVERSION).dylib
  CFLAGS := -DMACOSX -D_DARWIN_C_SOURCE $(CFLAGS)
else
ifeq ($(JARSUFFIX), win32)
  SHAREDLIBRARY = miniupnpc.dll
else
  # Linux/BSD/etc.
  SHAREDLIBRARY = libminiupnpc.so
  SONAME = $(SHAREDLIBRARY).$(APIVERSION)
endif
endif

EXECUTABLES = upnpc-static
EXECUTABLES_ADDTESTS = testminixml minixmlvalid testupnpreplyparse \
			  testigddescparse testminiwget

TESTMINIXMLOBJS = minixml.o igd_desc_parse.o testminixml.o

TESTMINIWGETOBJS = miniwget.o testminiwget.o connecthostport.o receivedata.o

TESTUPNPREPLYPARSE = testupnpreplyparse.o minixml.o upnpreplyparse.o

TESTIGDDESCPARSE = testigddescparse.o igd_desc_parse.o minixml.o \
                   miniupnpc.o miniwget.o upnpcommands.o upnpreplyparse.o \
                   minisoap.o connecthostport.o receivedata.o \
                   portlistingparse.o

ifneq ($(OS), AmigaOS)
EXECUTABLES := $(EXECUTABLES) upnpc-shared
TESTMINIWGETOBJS := $(TESTMINIWGETOBJS) minissdpc.o
TESTIGDDESCPARSE := $(TESTIGDDESCPARSE) minissdpc.o
endif

LIBDIR ?= lib
# install directories
INSTALLPREFIX ?= $(PREFIX)/usr
INSTALLDIRINC = $(INSTALLPREFIX)/include/miniupnpc
INSTALLDIRLIB = $(INSTALLPREFIX)/$(LIBDIR)
INSTALLDIRBIN = $(INSTALLPREFIX)/bin
INSTALLDIRMAN = $(INSTALLPREFIX)/share/man

FILESTOINSTALL = $(LIBRARY) $(EXECUTABLES)
ifneq ($(OS), AmigaOS)
FILESTOINSTALL := $(FILESTOINSTALL) $(SHAREDLIBRARY)
endif


.PHONY:	install clean depend all check test everything \
	installpythonmodule updateversion
#	validateminixml validateminiwget

all:	$(LIBRARY) $(EXECUTABLES)

test:	check

check:	validateminixml validateminiwget validateupnpreplyparse

everything:	all $(EXECUTABLES_ADDTESTS)

pythonmodule:	$(LIBRARY) miniupnpcmodule.c setup.py
	python setup.py build
	touch $@

installpythonmodule:	pythonmodule
	python setup.py install

pythonmodule3:	$(LIBRARY) miniupnpcmodule.c setup.py
	python3 setup.py build
	touch $@

installpythonmodule3:	pythonmodule3
	python3 setup.py install

validateminixml:	minixmlvalid
	@echo "minixml validation test"
	./minixmlvalid
	touch $@

validateminiwget:	testminiwget minihttptestserver testminiwget.sh
	@echo "miniwget validation test"
	./testminiwget.sh
	touch $@

validateupnpreplyparse:	testupnpreplyparse testupnpreplyparse.sh
	@echo "upnpreplyparse validation test"
	./testupnpreplyparse.sh
	touch $@

clean:
	$(RM) $(LIBRARY) $(SHAREDLIBRARY) $(EXECUTABLES) $(OBJS) miniupnpcstrings.h
	# clean python stuff
	$(RM) pythonmodule pythonmodule3
	$(RM) validateminixml validateminiwget validateupnpreplyparse
	$(RM) -r build/ dist/
	#python setup.py clean
	# clean jnaerator stuff
	$(RM) _jnaerator.* java/miniupnpc_$(OS).jar

distclean: clean
	$(RM) $(JNAERATOR) java/*.jar java/*.class out.errors.txt

updateversion:	miniupnpc.h
	cp miniupnpc.h miniupnpc.h.bak
	sed 's/\(.*MINIUPNPC_API_VERSION\s\+\)[0-9]\+/\1$(APIVERSION)/' < miniupnpc.h.bak > miniupnpc.h

install:	updateversion $(FILESTOINSTALL)
	$(INSTALL) -d $(DESTDIR)$(INSTALLDIRINC)
	$(INSTALL) -m 644 $(HEADERS) $(DESTDIR)$(INSTALLDIRINC)
	$(INSTALL) -d $(DESTDIR)$(INSTALLDIRLIB)
	$(INSTALL) -m 644 $(LIBRARY) $(DESTDIR)$(INSTALLDIRLIB)
ifneq ($(OS), AmigaOS)
	$(INSTALL) -m 644 $(SHAREDLIBRARY) $(DESTDIR)$(INSTALLDIRLIB)/$(SONAME)
	ln -fs $(SONAME) $(DESTDIR)$(INSTALLDIRLIB)/$(SHAREDLIBRARY)
endif
	$(INSTALL) -d $(DESTDIR)$(INSTALLDIRBIN)
ifeq ($(OS), AmigaOS)
	$(INSTALL) -m 755 upnpc-static $(DESTDIR)$(INSTALLDIRBIN)/upnpc
else
	$(INSTALL) -m 755 upnpc-shared $(DESTDIR)$(INSTALLDIRBIN)/upnpc
endif
	$(INSTALL) -m 755 external-ip.sh $(DESTDIR)$(INSTALLDIRBIN)/external-ip
ifneq ($(OS), AmigaOS)
	$(INSTALL) -d $(DESTDIR)$(INSTALLDIRMAN)/man3
	$(INSTALL) man3/miniupnpc.3 $(DESTDIR)$(INSTALLDIRMAN)/man3/miniupnpc.3
ifeq ($(OS), Linux)
	gzip $(DESTDIR)$(INSTALLDIRMAN)/man3/miniupnpc.3
endif
endif


cleaninstall:
	$(RM) -r $(DESTDIR)$(INSTALLDIRINC)
	$(RM) $(DESTDIR)$(INSTALLDIRLIB)/$(LIBRARY)
	$(RM) $(DESTDIR)$(INSTALLDIRLIB)/$(SHAREDLIBRARY)

depend:
	makedepend -Y -- $(CFLAGS) -- $(SRCS) 2>/dev/null

$(LIBRARY):	$(LIBOBJS)
	$(AR) crs $@ $?

$(SHAREDLIBRARY):	$(LIBOBJS)
ifeq ($(OS), Darwin)
#	$(CC) -dynamiclib $(LDFLAGS) -Wl,-install_name,$(SONAME) -o $@ $^
	$(CC) -dynamiclib $(LDFLAGS) -Wl,-install_name,$(INSTALLDIRLIB)/$(SONAME) -o $@ $^
else
	$(CC) -shared $(LDFLAGS) -Wl,-soname,$(SONAME) -o $@ $^
endif

upnpc-static:	upnpc.o $(LIBRARY)
	$(CC) $(LDFLAGS) -o $@ $^

upnpc-shared:	upnpc.o $(SHAREDLIBRARY)
	$(CC) $(LDFLAGS) -o $@ $^

testminixml:	$(TESTMINIXMLOBJS)

testminiwget:	$(TESTMINIWGETOBJS)

minixmlvalid:	minixml.o minixmlvalid.o

testupnpreplyparse:	$(TESTUPNPREPLYPARSE)

testigddescparse:	$(TESTIGDDESCPARSE)

miniupnpcstrings.h:	miniupnpcstrings.h.in updateminiupnpcstrings.sh VERSION
	$(SH) updateminiupnpcstrings.sh

# ftp tool supplied with OpenBSD can download files from http.
jnaerator-%.jar:
	wget $(JNAERATORBASEURL)/$@ || \
	curl -o $@ $(JNAERATORBASEURL)/$@ || \
	ftp $(JNAERATORBASEURL)/$@

jar: $(SHAREDLIBRARY)  $(JNAERATOR)
	$(JAVA) -jar $(JNAERATOR) $(JNAERATORARGS) \
	miniupnpc.h declspec.h upnpcommands.h upnpreplyparse.h \
	igd_desc_parse.h miniwget.h upnperrors.h $(SHAREDLIBRARY) \
	-package fr.free.miniupnp -o . -jar java/miniupnpc_$(JARSUFFIX).jar -v

mvn_install:
	mvn install:install-file -Dfile=java/miniupnpc_$(JARSUFFIX).jar \
	 -DgroupId=com.github \
	 -DartifactId=miniupnp \
	 -Dversion=$(VERSION) \
	 -Dpackaging=jar \
	 -Dclassifier=$(JARSUFFIX) \
	 -DgeneratePom=true \
	 -DcreateChecksum=true

# make .deb packages
deb: /usr/share/pyshared/stdeb all
	(python setup.py --command-packages=stdeb.command bdist_deb)

# install .deb packages
ideb:
	(sudo dpkg -i deb_dist/*.deb)

/usr/share/pyshared/stdeb: /usr/share/doc/python-all-dev
	(sudo apt-get install python-stdeb)

/usr/share/doc/python-all-dev:
	(sudo apt-get install python-all-dev)

minihttptestserver:	minihttptestserver.o

# DO NOT DELETE THIS LINE -- make depend depends on it.

igd_desc_parse.o: igd_desc_parse.h
miniupnpc.o: miniupnpc.h declspec.h igd_desc_parse.h minissdpc.h miniwget.h
miniupnpc.o: minisoap.h minixml.h upnpcommands.h upnpreplyparse.h
miniupnpc.o: portlistingparse.h miniupnpctypes.h connecthostport.h
miniupnpc.o: receivedata.h
minixml.o: minixml.h
minisoap.o: minisoap.h miniupnpcstrings.h
miniwget.o: miniupnpcstrings.h miniwget.h declspec.h connecthostport.h
miniwget.o: receivedata.h
upnpc.o: miniwget.h declspec.h miniupnpc.h igd_desc_parse.h upnpcommands.h
upnpc.o: upnpreplyparse.h portlistingparse.h miniupnpctypes.h upnperrors.h
upnpcommands.o: upnpcommands.h upnpreplyparse.h portlistingparse.h declspec.h
upnpcommands.o: miniupnpctypes.h miniupnpc.h igd_desc_parse.h
upnpreplyparse.o: upnpreplyparse.h minixml.h
testminixml.o: minixml.h igd_desc_parse.h
minixmlvalid.o: minixml.h
testupnpreplyparse.o: upnpreplyparse.h
minissdpc.o: minissdpc.h miniupnpc.h declspec.h igd_desc_parse.h codelength.h
upnperrors.o: upnperrors.h declspec.h upnpcommands.h upnpreplyparse.h
upnperrors.o: portlistingparse.h miniupnpctypes.h miniupnpc.h
upnperrors.o: igd_desc_parse.h
testigddescparse.o: igd_desc_parse.h minixml.h miniupnpc.h declspec.h
testminiwget.o: miniwget.h declspec.h
connecthostport.o: connecthostport.h
receivedata.o: receivedata.h
