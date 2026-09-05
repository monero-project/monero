#!/usr/bin/env python3
"""Deterministic, defensive GPT-Doug preflight diagnosis from redacted health JSON."""
import json
import sys

data=json.load(sys.stdin)
status=data.get("status")
if status=="healthy":
    diagnosis=("ready","Node is synchronized and reachable.","Continue monitoring; no operator action required.")
elif status=="degraded":
    diagnosis=("attention","Node is reachable but not synchronized.","Check peer connectivity, disk space, clock accuracy, and daemon logs.")
else:
    diagnosis=("blocked","Local RPC is unreachable.","Confirm monerod is running and RPC remains bound to an approved local interface.")
print(json.dumps({
 "agent":"gpt-doug-llm",
 "mode":"defensive-preflight",
 "decision":diagnosis[0],
 "summary":diagnosis[1],
 "recommendedAction":diagnosis[2],
 "humanApprovalRequired":True
},indent=2))
