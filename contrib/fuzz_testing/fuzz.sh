#!/bin/sh

AFLFUZZ=$(which afl-fuzz)
if ! test -x "$AFLFUZZ"
then
  echo "afl-fuzz not found - install american-fuzzy-lop"
  exit 1
fi

type="$1"
if test -z "$type"
then
  echo "usage: $0 block|transaction|signature|cold-outputs|cold-transaction"
  exit 1
fi
case "$type" in
  block|transaction|signature|cold-outputs|cold-transaction) ;;
  *) echo "usage: $0 block|transaction|signature|cold-outputs|cold-transaction"; exit 1 ;;
esac

afl-fuzz -i tests/data/fuzz/$type -m 150 -t 250 -o fuzz-out/$type build/fuzz/tests/fuzz/${type}_fuzz_tests
