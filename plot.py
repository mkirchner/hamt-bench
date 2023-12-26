import sqlite3
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import numpy as np


def trim(arr, trim):
    """
    Trim the top and bottom 0.5*TRIM*100% values from ARR.
    """
    n = len(arr)
    k = int(round(n * float(trim) / 2))
    s = sorted(arr)
    return s[k+1:n-k]

def subplots(i, j):
    # subplots with a consistent return type (always a numpy array)
    fig, axs = plt.subplots(i, j)  #, layout='constrained')
    if not isinstance(axs, np.ndarray):
        axs = np.asarray([axs])
    return fig, axs


def query_stats():
    query = """SELECT product || ':' || gitcommit || ':' || substr(benchmark, 1, 4) as id, measurement, scale, repeat, ns FROM numbers;"""
    conn = sqlite3.connect("db/db.sqlite")
    df2 = pd.read_sql_query(query, conn)
    conn.close()
    labels = df2["id"].unique()
    scales = df2["scale"].unique()
    measurements = df2["measurement"].unique()
    return df2, labels, scales, measurements

def create_plot():
    df2, labels, scales, measurements = query_stats()
    fig, axs = subplots(1, len(measurements))
    labels = sorted(labels)

    # explicitly create default handle sequence
    colors = plt.rcParams['axes.prop_cycle'].by_key()['color']
    cmap = { label: i for i, label in enumerate(labels) }
    dhs = [ mlines.Line2D([], [], color=colors[i], label=label) for i, label in enumerate(labels)]

    for m_ix, measurement in enumerate(measurements):
        handles = []
        for label in labels:
            times = [np.nan] * len(scales)
            time_cis = [0.0] * len(scales)
            handle = dhs[cmap[label]]
            for ix, scale in enumerate(sorted(scales)):
                data = df2.loc[
                    (df2["id"] == label)
                    & (df2["scale"] == scale)
                    & (df2["measurement"] == measurement)
                ]["ns"]
                if not data.empty:
                    trimmed_times = trim(data, 0.4)
                    times[ix] = np.mean(trimmed_times)
                    time_cis[ix] = 3*np.std(trimmed_times)
                    if handle not in handles:
                        handles.append(dhs[cmap[label]])
                else:
                    times[ix], time_cis[ix] = np.nan, np.nan
            #axs[m_ix].plot(scales, times, "o-", label=label)
            # if not any(np.isnan(t) for t in times):
            axs[m_ix].errorbar(scales, times, yerr=time_cis, fmt=".-", label=label)
        axs[m_ix].set_xscale("log")
        axs[m_ix].set_ylim(0, 750)
        axs[m_ix].set_title(measurement)
        #print("=======")
        #print(handles)
        axs[m_ix].legend(handles=handles, fontsize=8, loc='upper left', frameon=False)
        axs[m_ix].spines[['right', 'top']].set_visible(False)

    axs[0].set_ylabel("ns/op")
    # axs[0].legend()
    #handles, labels = axs[0].get_legend_handles_labels()
    #fig.legend(handles, labels, loc='outside left upper', frameon=False, draggable=True)
    # fig.tight_layout()
    plt.show()
    fig.savefig("benchmark.png", dpi=300, bbox_inches='tight')

def main():
    create_plot()

if __name__ == "__main__":
    main()
