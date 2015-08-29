#!/bin/sh
#
# This script embeds net_skeleton directly into the source file.
# The source file must have a placeholder for the net_skeleton code,
# two following lines:

# // net_skeleton start
# // net_skeleton end
#
# Net skeleton code will be inserted between those two lines.

if ! test -f "$1" ; then
  echo "Usage: $0 <source_file>"
  exit 1
fi

D=`dirname $0`

F1=$D/../net_skeleton.h
F2=$D/../net_skeleton.c

sed '/#include "net_skeleton.h"/d' $F2 > /tmp/$$
F2=/tmp/$$

A='\/\/ net_skeleton start'
B='\/\/ net_skeleton end'

sed -i .$$.bak -e "/$A/,/$B/ { /$A/{ n; r $F1" -e "r $F2" -e "}; /$B/!d; }" "$1"
