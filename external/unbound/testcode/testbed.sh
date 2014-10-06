#!/usr/bin/env bash
# Testbed for NSD.
# By Wouter Wijngaards, NLnet Labs, 2006.
# BSD License.

# this version prefers gmake if available.
# adds variable LDNS for the LDNS path to use.

# global settings
CONFIGURE_FLAGS=""
REPORT_FILE=testdata/testbed.report
LOG_FILE=testdata/testbed.log
HOST_FILE=testdata/host_file.$USER

if test ! -f $HOST_FILE; then
	echo "No such file: $HOST_FILE"
	exit 1
fi

function echossh() # like ssh but echos.
{
	echo "> ssh $*"
	ssh $*
}

# Compile and run NSD on platforms
function dotest() 
# parameters: <host> <dir>
# host is name of ssh host
# dir is directory of nsd trunk on host
{
	echo "$1 begin on "`date` | tee -a $REPORT_FILE

	DISABLE=""
	if test $IP6 = no; then
		DISABLE="--disable-ipv6"
	fi
	if test x$LDNS != x; then
		DISABLE="--with-ldns=$LDNS $DISABLE"
	fi
	if test x$LIBEVENT != x; then
		DISABLE="--with-libevent=$LIBEVENT $DISABLE"
	fi

	cat >makeconf.mak.$$ << EOF 
#configure:	configure.ac
#	$AC_CMD
#	touch configure
Makefile:	Makefile.in #configure
	./configure $CONFIGURE_FLAGS $DISABLE
	touch Makefile 
EOF
	scp makeconf.mak.$$ $1:$2
	# determine make to use
	tempx=`ssh $1 "cd $2; which gmake"`
	MAKE_CMD=`ssh $1 "cd $2; if test -f '$tempx'; then echo $tempx; else echo $MAKE_CMD; fi"`

	if test $SVN = yes; then
		echossh $1 "cd $2; svn up"
		echossh $1 "cd $2; $MAKE_CMD -f makeconf.mak.$$ configure"
	else
		# svn and autoconf locally
		echo "fake svn via svnexport, tar, autoconf, bison, flex."
		svn export svn+ssh://open.nlnetlabs.nl/svn/nsd/trunk unbound_ttt
		(cd unbound_ttt; $AC_CMD; rm -r autom4te* .c-mode-rc.el .cvsignore)
		if test $FIXCONFIGURE = yes; then
			echo fixing up configure length test.
			(cd unbound_ttt; mv configure oldconf; sed -e 's?while (test "X"?lt_cv_sys_max_cmd_len=65500; echo skip || while (test "X"?' <oldconf >configure; chmod +x ./configure)
		fi
		du unbound_ttt
		rsync -vrcpz --rsync-path=/home/wouter/bin/rsync unbound_ttt $1:unbound_ttt
		# tar czf unbound_ttt.tgz unbound_ttt
		rm -rf unbound_ttt
		# ls -al unbound_ttt.tgz
		# scp unbound_ttt.tgz $1:unbound_ttt.tar.gz
		# rm unbound_ttt.tgz
		# echossh $1 "gtar xzf unbound_ttt.tar.gz && rm unbound_ttt.tar.gz"
	fi
	echossh $1 "cd $2; $MAKE_CMD -f makeconf.mak.$$ Makefile"
	echossh $1 "cd $2; $MAKE_CMD all tests"
	echossh $1 "cd $2; $MAKE_CMD doc"
	if test $RUN_TEST = yes; then
	echossh $1 "cd $2; bash testcode/do-tests.sh"
	echossh $1 "cd $2/testdata; sh ../testcode/mini_tpkg.sh -q report" | tee -a $REPORT_FILE
	fi
	echossh $1 "cd $2; rm -f makeconf.mak.$$"
	rm -f makeconf.mak.$$
	echo "$1 end on "`date` | tee -a $REPORT_FILE
}

echo "on "`date`" by $USER." > $REPORT_FILE
echo "on "`date`" by $USER." > $LOG_FILE

# read host names
declare -a hostname desc dir vars
IFS='	'
i=0
while read a b c d; do
	if echo $a | grep "^#" >/dev/null; then
		continue # skip it
	fi
	# append after arrays
	hostname[$i]=$a
	desc[$i]=$b
	dir[$i]=$c
	vars[$i]=$d
	i=$(($i+1))
done <$HOST_FILE
echo "testing on $i hosts"

# do the test
for((i=0; i<${#hostname[*]}; i=$i+1)); do
	if echo ${hostname[$i]} | grep "^#" >/dev/null; then
		continue # skip it
	fi
	# echo "hostname=[${hostname[$i]}]"
	# echo "desc=[${desc[$i]}]"
	# echo "dir=[${dir[$i]}]"
	# echo "vars=[${vars[$i]}]"
	AC_CMD="libtoolize -c --force; autoconf && autoheader"
	MAKE_CMD="make"
	SVN=yes
	IP6=yes
	FIXCONFIGURE=no
	RUN_TEST=yes
	LDNS=
	LIBEVENT=
	eval ${vars[$i]}
	echo "*** ${hostname[$i]} ${desc[$i]} ***" | tee -a $LOG_FILE | tee -a $REPORT_FILE
	dotest ${hostname[$i]} ${dir[$i]} 2>&1 | tee -a $LOG_FILE
done

echo "done"
