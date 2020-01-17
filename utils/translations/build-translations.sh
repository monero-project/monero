#!/bin/sh

lrelease=`which lrelease 2> /dev/null`
if test -z "$lrelease"
then
  lrelease=`which lrelease-qt4 2> /dev/null`
fi
if test -z "$lrelease"
then
  lrelease=`which lrelease-qt5 2> /dev/null`
fi
if test -z "$lrelease"
then
  echo "lrelease not found"
  exit 1
fi

echo "using $lrelease"
if test -f translations/ready
then
  languages=""
  for language in $(cat translations/ready)
  do
    languages="$languages translations/monero_$language.ts"
  done
else
  languages="translations/*.ts"
fi
"$lrelease" $languages

