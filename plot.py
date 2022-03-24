import sqlite3
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


def trimmed_mean(arr, percent):
    n = len(arr)
    k = int(round(n * (float(percent) / 100) / 2))
    return np.mean(arr[k + 1 : n - k])


def subplots(i, j):
    # subplots with a consistent return type (always a numpy array)
    fig, axs = plt.subplots(i, j)
    if not isinstance(axs, np.ndarray):
        axs = np.asarray([axs])
    return fig, axs


query = """SELECT gitcommit || ':' || substr(benchmark, 1, 4) as id, measurement, scale, repeat, ns FROM numbers;"""
conn = sqlite3.connect("db/db.sqlite")
df2 = pd.read_sql_query(query, conn)

labels = df2["id"].unique()
scales = df2["scale"].unique()
measurements = df2["measurement"].unique()

fig, axs = subplots(1, len(measurements))

for m_ix, measurement in enumerate(measurements):
    for label in labels:
        times = [0.0] * len(scales)
        for ix, scale in enumerate(sorted(scales)):
            data = df2.loc[
                (df2["id"] == label)
                & (df2["scale"] == scale)
                & (df2["measurement"] == measurement)
            ]["ns"]
            times[ix] = trimmed_mean(data, 0.4)
        axs[m_ix].plot(scales, times, "o-", label=label)
    axs[m_ix].set_xscale("log")
    axs[m_ix].set_ylim(0, 1800)
    axs[m_ix].set_title(measurement)

axs[0].set_ylabel("ns/op")
axs[0].legend()

plt.show()
# plt.tight_layout()
fig.savefig("benchmark.png", dpi=300)
