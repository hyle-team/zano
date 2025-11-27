#!/usr/bin/env python3
import os
import csv
from datetime import datetime

import matplotlib.pyplot as plt

LOGFILE = os.path.expanduser("~/zano_connections.csv")

def main():
    if not os.path.exists(LOGFILE):
        print(f"File {LOGFILE} not found. Please run monitor_zano.py first.")
        return

    timestamps = []
    incoming = []
    outgoing = []

    with open(LOGFILE, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            ts = int(row["timestamp"])
            timestamps.append(datetime.fromtimestamp(ts))
            incoming.append(int(row["incoming"]))
            outgoing.append(int(row["outgoing"]))

    if not timestamps:
        print("В файле нет данных.")
        return

    plt.figure()
    plt.plot(timestamps, incoming, label="incoming")
    plt.plot(timestamps, outgoing, label="outgoing")
    plt.xlabel("Time")
    plt.ylabel("Connections count")
    plt.title("Zano node connections")
    plt.legend()
    plt.tight_layout()

    # показать окно с графиком
    plt.show()

if __name__ == "__main__":
    main()
