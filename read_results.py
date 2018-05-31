# coding = utf-8

import os
import re
import sys
import pandas as pd

resultsFolder = "/mnt/e/Hybrid"

pattern = "^(real)\t(\d+m\d+.\d+s)"
time_pattern = "^(\d+)m(\d+.\d+)s"
filename_pattern = "^.*_(\w*(\d+)|(\d+).*).txt_(\d+)_(\d+)"

prog = re.compile(pattern)
prog2 = re.compile(time_pattern)
reg = re.compile(filename_pattern)

def main():
    data = []
    resultFiles = [f for f in os.listdir(resultsFolder) if os.path.isfile(os.path.join(resultsFolder, f)) and ".e" in f ]
    for resultFile in resultFiles:
        totalTime = 0
        nameMatch = reg.match(resultFile)
        with open(os.path.join(resultsFolder, resultFile), 'r') as f:
            for line in f.readlines():
                match = prog.match(line)
                if match:
                    match2 = prog2.match(match.group(2))
                    time = 0
                    if match2:
                        if float(match2.group(1)) > 0:
                            time += float(match2.group(1)) * 60
                        time += float(match2.group(2))
                    totalTime += time
        data.append((nameMatch.group(2) if nameMatch.group(2) else nameMatch.group(3), nameMatch.group(4), nameMatch.group(5), totalTime))
    df = pd.DataFrame.from_records(data, columns=["trees", "MPI_Nodes", "OMP_Threads", "time"])
    df.to_csv(os.path.join(resultsFolder, "data.csv"), index=False)
    # df.loc[(df.program == 'send')].to_csv(os.path.join(resultsFolder, "send.csv"), index=False)
    # df.loc[(df.program == 'bcast')].to_csv(os.path.join(resultsFolder, "bcast.csv"), index=False)
    # sys.exit(0)

if __name__ == '__main__':
    main()
