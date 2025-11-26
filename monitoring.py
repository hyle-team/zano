#!/usr/bin/env python3
import time
import json
import os
import requests

RPC_URL = "http://127.0.0.1:11211/json_rpc"
LOGFILE = os.path.expanduser("~/zano_connections.csv")
INTERVAL_SECONDS = 5   # how often to poll, you can change

payload = {
    "jsonrpc": "2.0",
    "id": 0,
    "method": "getinfo",
    "params": {"flags": 1048575},
}

def main():
    # if file doesnt exist, create it with header
    if not os.path.exists(LOGFILE):
        with open(LOGFILE, "w") as f:
            f.write("timestamp,incoming,outgoing\n")

    print(f"Logging to {LOGFILE}, interval {INTERVAL_SECONDS} sec. Ctrl+C to stop.")
    while True:
        try:
            resp = requests.post(RPC_URL, json=payload, timeout=5)
            resp.raise_for_status()
            data = resp.json()["result"]

            ts = int(time.time())
            incoming = data.get("incoming_connections_count", 0)
            outgoing = data.get("outgoing_connections_count", 0)

            line = f"{ts},{incoming},{outgoing}"
            print(line)

            with open(LOGFILE, "a") as f:
                f.write(line + "\n")

            time.sleep(INTERVAL_SECONDS)
        except KeyboardInterrupt:
            print("\nStopped by user.")
            break
        except Exception as e:
            print(f"Request error: {e}")
            time.sleep(INTERVAL_SECONDS)

if __name__ == "__main__":
    main()
