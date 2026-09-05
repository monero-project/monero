#!/usr/bin/env python3
"""Read-only local monerod health probe. Emits redacted operational JSON."""
import json
import os
import time
import urllib.error
import urllib.request

url = os.getenv("XUNIA_MONEROD_RPC", "http://127.0.0.1:18081/json_rpc")
request = urllib.request.Request(
    url,
    data=json.dumps({"jsonrpc":"2.0","id":"xunia-health","method":"get_info"}).encode(),
    headers={"Content-Type":"application/json"},
)
try:
    with urllib.request.urlopen(request, timeout=5) as response:
        result = json.load(response).get("result", {})
    output = {
        "schemaVersion":"1.0.0",
        "observedAt":int(time.time()),
        "status":"healthy" if result.get("synchronized") else "degraded",
        "network":("testnet" if result.get("testnet") else "stagenet" if result.get("stagenet") else "mainnet"),
        "height":result.get("height"),
        "targetHeight":result.get("target_height"),
        "peerCount":int(result.get("incoming_connections_count",0))+int(result.get("outgoing_connections_count",0)),
        "offline":result.get("offline"),
        "busySyncing":result.get("busy_syncing")
    }
except (urllib.error.URLError, TimeoutError, ValueError) as exc:
    output={"schemaVersion":"1.0.0","observedAt":int(time.time()),"status":"unreachable","errorType":type(exc).__name__}
print(json.dumps(output, sort_keys=True))
