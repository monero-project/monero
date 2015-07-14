#!/bin/sh

lrelease=`which lrelease 2> /dev/null`
if test -z "$lrelease"
then
  lrelease=`which lrelease-qt4 2> /dev/null`
fi
if test -z "$lrelease"
then
  echo "lrelease not found"
  exit 1
fi

echo "using $lrelease"
"$lrelease" translations/*.ts

