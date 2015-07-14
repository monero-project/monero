#!/bin/sh

lupdate=`which lupdate 2> /dev/null`
if test -z "$lupdate"
then
  lupdate=`which lupdate-qt4 2> /dev/null`
fi
if test -z "$lupdate"
then
  echo "lupdate not found"
  exit 1
fi

echo "using $lupdate"
"$lupdate" `find src -name \*.cpp` -ts translations/*.ts

