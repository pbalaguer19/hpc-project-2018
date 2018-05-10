# coding = utf-8

import os
import re
import sys
import pandas as pd

resultsFolder = "/mnt/e/MPI"

pattern = "^(user|sys)\t(\d+m\d+.\d+s)"
time_pattern = "^(\d+)m(\d+.\d+)s"
filename_pattern = "^.*(bcast|send)_(\w*(\d+)|(\d+).*).txt_(\d+)"

prog = re.compile(pattern)
prog2 = re.compile(time_pattern)
reg = re.compile(filename_pattern)

def main():
    data = []
    resultFiles = [f for f in os.listdir(resultsFolder) if os.path.isfile(os.path.join(resultsFolder, f)) and ".e" in f ]
    totalTime = 0
    for resultFile in resultFiles:
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
        
        data.append((nameMatch.group(1), nameMatch.group(3) if nameMatch.group(3) else nameMatch.group(4), nameMatch.group(5), totalTime))
    df = pd.DataFrame.from_records(data, columns=["program", "trees", "cores", "time"])
    df.to_csv(os.path.join(resultsFolder, "data.csv"), index=False)
        # sys.exit(0)

if __name__ == '__main__':
    main()

