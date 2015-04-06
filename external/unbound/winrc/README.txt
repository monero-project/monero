README for Unbound on Windows.

(C) 2009, W.C.A. Wijngaards, NLnet Labs.

See LICENSE for the license text file.


+++ Introduction

Unbound is a recursive DNS server.  It does caching, full recursion, stub
recursion, DNSSEC validation, NSEC3, IPv6.  More information can be found 
at the http://unbound.net site.  Unbound has been built and tested on 
Windows XP, Vista, 7 and 8.

At http://unbound.net/documentation is an install and configuration manual
for windows.

email: unbound-bugs@nlnetlabs.nl


+++ How to use it

In ControlPanels\SystemTasks\Services you can start/stop the daemon.
In ControlPanels\SystemTasks\Logbooks you can see log entries (unless you
configured unbound to log to file).

By default the daemon provides service only to localhost.  See the manual
on how to change that (you need to edit the config file).

To change options, edit the service.conf file.  The example.conf file 
contains information on the various configuration options.  The config
file is the same as on Unix.  The options log-time-ascii, chroot, username
and pidfile are not supported on windows.


+++ How to compile

Unbound is open source under the BSD license.  You can compile it yourself.

1. Install MinGW and MSYS.  http://www.mingw.org
This is a free, open source, compiler and build environment.
Note, if your username contains a space, create a directory
C:\msys\...\home\user to work in (click on MSYS; type: mkdir /home/user ).

2. Install openssl, or compile it yourself.  http://www.openssl.org
Unbounds need the header files and libraries.  Static linking makes
things easier.  This is an open source library for cryptographic functions.
And libexpat is needed.

3. Compile Unbound
Get the source code tarball  http://unbound.net
Move it into the C:\msys\...\home\user directory.
Double click on the MSYS icon and give these commands
$ cd /home/user
$ tar xzvf unbound-xxx.tar.gz
$ cd unbound-xxx
$ ./configure --enable-static-exe
If you compiled openssl yourself, pass --with-ssl=../openssl-xxx too.
If you compiled libexpat yourself, pass --with-libexpat=../expat-install too.
The configure options for libevent or threads are not applicable for 
windows, because builtin alternatives for the windows platform are used.
$ make
And you have unbound.exe

If you run unbound-service-install.exe (double click in the explorer),
unbound is installed as a service in the controlpanels\systemtasks\services,
from the current directory. unbound-service-remove.exe uninstalls the service.

Unbound and its utilities also work from the commandline (like on unix) if 
you prefer.


+++ Cross compile

You can crosscompile unbound.  This results in .exe files.
Install the packages: mingw32-binutils mingw32-cpp mingw32-filesystem 
mingw32-gcc mingw32-openssl mingw32-openssl-static mingw32-runtime zip
mingw32-termcap mingw32-w32api mingw32-zlib mingw32-zlib-static mingw32-nsis
(package names for fedora 11).

For dynamic linked executables
$ mingw32-configure
$ make
$ mkdir /home/user/installdir
$ make install DESTDIR=/home/user/installdir
Find the dlls and exes in /home/user/installdir and
crypto in /usr/i686-pc-mingw32/sys-root/mingw/bin

For static linked executables
Use --enable-staticexe for mingw32-configure, see above. Or use makedist.sh,
copy System.dll from the windows dist of NSIS to /usr/share/nsis/Plugins/
Then do ./makedist.sh -w and the setup.exe is created using nsis.


+++ CREDITS

Unbound was written in portable C by Wouter Wijngaards (NLnet Labs).
See the CREDITS file in the source package for more contributor information.
Email unbound-bugs@nlnetlabs.nl

