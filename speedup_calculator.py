# coding = utf8

import pandas as pd
import numpy as np

SEND_FILE = 'results/MPI/send.csv'
BCAST_FILE = 'results/MPI/bcast.csv'
trees = [3, 6, 15, 20, 25, 27]
processes = [2, 4, 8, 16, 32]

def main():
    send = pd.read_csv(SEND_FILE, sep=",")
    bcast = pd.read_csv(BCAST_FILE, sep=",")
    send["SpeedUp"] = np.nan
    send["Efficiency"] = np.nan
    bcast["SpeedUp"] = np.nan
    bcast["Efficiency"] = np.nan

    approaches = [bcast, send]

    for tree in trees:
        for approach in approaches:
            serial_row = None
            for index, row in approach.loc[(send.trees == tree)].iterrows():
                if row.processes == 1:
                    serial_row = row
                else:
                    speedup = serial_row.time / row.time
                    approach.ix[index, 'SpeedUp'] = speedup
                    approach.ix[index, 'Efficiency'] = speedup / row.processes



    print(send.tail(15))
    #print(bcast.tail(10))



if __name__ == '__main__':
    main()
