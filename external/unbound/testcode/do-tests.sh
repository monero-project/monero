#!/usr/bin/env bash
. testdata/common.sh

NEED_SPLINT='00-lint.tpkg'
NEED_DOXYGEN='01-doc.tpkg'
NEED_XXD='fwd_compress_c00c.tpkg fwd_zero.tpkg'
NEED_NC='fwd_compress_c00c.tpkg fwd_zero.tpkg'
NEED_CURL='06-ianaports.tpkg root_anchor.tpkg'
NEED_WHOAMI='07-confroot.tpkg'
NEED_IPV6='fwd_ancil.tpkg fwd_tcp_tc6.tpkg stub_udp6.tpkg edns_cache.tpkg'
NEED_NOMINGW='tcp_sigpipe.tpkg 07-confroot.tpkg 08-host-lib.tpkg fwd_ancil.tpkg'

# test if dig and ldns-testns are available.
test_tool_avail "dig"
test_tool_avail "ldns-testns"

# test for ipv6, uses streamptcp peculiarity.
if ./streamtcp -f ::1 2>&1 | grep "not supported" >/dev/null 2>&1; then
	HAVE_IPV6=no
else
	HAVE_IPV6=yes
fi

# test mingw. no signals and so on.
if uname | grep MINGW >/dev/null; then
	HAVE_MINGW=yes
else
	HAVE_MINGW=no
fi

cd testdata;
sh ../testcode/mini_tpkg.sh clean
rm -f .perfstats.txt
for test in `ls *.tpkg`; do
	SKIP=0
	skip_if_in_list $test "$NEED_SPLINT" "splint"
	skip_if_in_list $test "$NEED_DOXYGEN" "doxygen"
	skip_if_in_list $test "$NEED_CURL" "curl"
	skip_if_in_list $test "$NEED_XXD" "xxd"
	skip_if_in_list $test "$NEED_NC" "nc"
	skip_if_in_list $test "$NEED_WHOAMI" "whoami"

	if echo $NEED_IPV6 | grep $test >/dev/null; then
		if test "$HAVE_IPV6" = no; then
			SKIP=1;
		fi
	fi
	if echo $NEED_NOMINGW | grep $test >/dev/null; then
		if test "$HAVE_MINGW" = yes; then
			SKIP=1;
		fi
	fi
	if test $SKIP -eq 0; then
		echo $test
		sh ../testcode/mini_tpkg.sh -a ../.. exe $test
	else
		echo "skip $test"
	fi
done
sh ../testcode/mini_tpkg.sh report
cat .perfstats.txt
