#!/bin/bash

DIR=$(realpath $(dirname $0))

echo "Checking monerod..."
monerod=""
for dir in \
  . \
  "$DIR" \
  "$DIR/../.." \
  "$DIR/build/release/bin" \
  "$DIR/../../build/release/bin" \
  "$DIR/build/Linux/master/release/bin" \
  "$DIR/../../build/Linux/master/release/bin" \
  "$DIR/build/Windows/master/release/bin" \
  "$DIR/../../build/Windows/master/release/bin"
do
  if test -x "$dir/monerod"
  then
    monerod="$dir/monerod"
    break
  fi
done
if test -z "$monerod"
then
  echo "monerod not found"
  exit 1
fi
echo "Found: $monerod"

TORDIR="$DIR/monero-over-tor"
TORRC="$TORDIR/torrc"
HOSTNAMEFILE="$TORDIR/hostname"
echo "Creating configuration..."
mkdir -p "$TORDIR"
chmod 700 "$TORDIR"
rm -f "$TORRC"
cat << EOF > "$TORRC"
ControlSocket $TORDIR/control
ControlSocketsGroupWritable 1
CookieAuthentication 1
CookieAuthFile $TORDIR/control.authcookie
CookieAuthFileGroupReadable 1
SocksPort 9050 IsolateDestAddr
SocksPort 9052 OnionTrafficOnly IsolateDestAddr
HiddenServiceDir $TORDIR
HiddenServicePort 18083 127.0.0.1:18083
HiddenServiceEnableIntroDoSDefense 1
HiddenServiceEnableIntroDoSRatePerSec 10
HiddenServiceEnableIntroDoSBurstPerSec 20
HiddenServicePoWDefensesEnabled 1
HiddenServicePoWQueueRate 5
HiddenServicePoWQueueBurst 10
HiddenServiceMaxStreams 1000
HiddenServiceMaxStreamsCloseCircuit 1
EOF

echo "Starting Tor..."
nohup tor -f "$TORRC" 2> "$TORDIR/tor.stderr" 1> "$TORDIR/tor.stdout" &
ready=0
for i in `seq 10`
do
  sleep 1
  if test -f "$HOSTNAMEFILE"
  then
    ready=1
    break
  fi
done
if test "$ready" = 0
then
  echo "Error starting Tor"
  cat "$TORDIR/tor.stdout"
  exit 1
fi

echo "Starting monerod..."
HOSTNAME=$(cat "$HOSTNAMEFILE")
"$monerod" \
  --anonymous-inbound "$HOSTNAME":18083,127.0.0.1:18083,25 --proxy 127.0.0.1:9050 --tx-proxy tor,127.0.0.1:9052,10 \
  --detach
ready=0
for i in `seq 10`
do
  sleep 1
  status=$("$monerod" status)
  echo "$status" | grep -q "Height:"
  if test $? = 0
  then
    ready=1
    break
  fi
done
if test "$ready" = 0
then
  echo "Error starting monerod"
  tail -n 400 "$HOME/.bitmonero/bitmonero.log" | grep -Ev stacktrace\|"Error: Couldn't connect to daemon:"\|"src/daemon/main.cpp:.*Monero\ \'" | tail -n 20
  exit 1
fi

echo "Ready. Your Tor hidden service is $HOSTNAME"
