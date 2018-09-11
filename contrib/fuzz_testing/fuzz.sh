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
  echo "usage: $0 block|transaction|signature|cold-outputs|cold-transaction|load-from-binary|load-from-json|base58|parse-url|http-client|levin|bulletproof"
  exit 1
fi
case "$type" in
  block|transaction|signature|cold-outputs|cold-transaction|load-from-binary|load-from-json|base58|parse-url|http-client|levin|bulletproof) ;;
  *) echo "usage: $0 block|transaction|signature|cold-outputs|cold-transaction|load-from-binary|load-from-json|base58|parse-url|http-client|levin|bulletproof"; exit 1 ;;
esac

if test -d "fuzz-out/$type"
then
  dir="-"
else
  dir="tests/data/fuzz/$type"
fi

mkdir -p fuzz-out
afl-fuzz -i "$dir" -m none -t 250 -o fuzz-out/$type build/fuzz/tests/fuzz/${type}_fuzz_tests @@
