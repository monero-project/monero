#!/bin/awk -f

function max(a, b) { if (a < b) return b; return a; }
function bytes_str(b) { if (b < 1024) return b " bytes"; if (b < 1024 * 1024) return b/1024 " kB"; return b/1024/1024 " MB"; }
function time_str(b) { if (b < 120) return b " sec"; if (b < 3600) return b/60 " min"; if (b < 86400) return b/3600 " hours"; return b/86400 " days"}

BEGIN {
commands["command-1001"] = "HANDSHAKE"
commands["command-1002"] = "TIMED_SYNC"
commands["command-1003"] = "PING"
commands["command-1004"] = "REQUEST_STAT_INFO"
commands["command-1005"] = "REQUEST_NETWORK_STATE"
commands["command-1006"] = "REQUEST_PEER_ID"
commands["command-1007"] = "REQUEST_SUPPORT_FLAGS"
commands["command-2001"] = "NOTIFY_NEW_BLOCK"
commands["command-2002"] = "NOTIFY_NEW_TRANSACTIONS"
commands["command-2003"] = "REQUEST_GET_OBJECTS"
commands["command-2004"] = "RESPONSE_GET_OBJECTS"
commands["command-2006"] = "NOTIFY_REQUEST_CHAIN"
commands["command-2007"] = "RESPONSE_CHAIN_ENTRY"
commands["command-2008"] = "NOTIFY_NEW_FLUFFY_BLOCK"
commands["command-2009"] = "NOTIFY_REQUEST_FLUFFY_MISSING_TX"
}

/	net.p2p.traffic	/ {
  date=gensub(/-/, " ", "g", $1)
  time=gensub(/\..*/, "", "g", gensub(/:/, " ", "g", $2))
  ip=gensub(/\[/, "", "g", $7)
  outin=gensub(/]/, "", "g", $8)
  timestamp=date " " time
  timestamp=mktime(timestamp)
  if (!t0)
    t0 = timestamp
  if (!t0ip[ip])
    t0ip[ip] = timestamp
  t1 = timestamp
  t1ip[ip] = timestamp
  bytes=$9
  dir=$11
  command=$14
  initiator=$17

  bytes_by_command[command] += bytes
  bytes_by_ip[ip] += bytes
  if (dir == "sent")
    bytes_sent_by_ip[ip] += bytes
  else
    bytes_received_by_ip[ip] += bytes
  bytes_by_outin[outin] += bytes
  bytes_by_direction[dir] += bytes
  bytes_by_initiator[initiator] += bytes
}

END {
  dt = t1 - t0
  print "Running time:", time_str(dt)
  for (command in bytes_by_command) {
    category = command
    if (commands[command])
      category = commands[command];
    print "Category", category ":", bytes_str(bytes_by_command[command])
  }
  for (direction in bytes_by_direction) print direction ":", bytes_str(bytes_by_direction[direction])
  for (initiator in bytes_by_initiator) print "Initiating from", initiator ":", bytes_str(bytes_by_initiator[initiator])
  for (outin in bytes_by_outin) print "With", outin, "peers:", bytes_str(bytes_by_outin[outin])
  for (ip in bytes_by_ip) print "IP", ip ":", bytes_str(bytes_by_ip[ip])
  print "Download rate:", bytes_str(bytes_by_direction["received"] / max(dt, 1)) "/s"
  for (ip in bytes_received_by_ip)
    print "    ", ip ":", bytes_str(bytes_received_by_ip[ip] / max(t1ip[ip] - t0ip[ip], 1)) "/s over", time_str(t1ip[ip] - t0ip[ip])
  print "Upload rate:", bytes_str(bytes_by_direction["sent"] / max(dt, 1)) "/s"
  for (ip in bytes_sent_by_ip)
    print "    ", ip ":", bytes_str(bytes_sent_by_ip[ip] / max(t1ip[ip] - t0ip[ip], 1)) "/s over", time_str(t1ip[ip] - t0ip[ip])
}
