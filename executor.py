# coding = utf8

import os
import sys 

testFolder = "testbed"
ncores = ['4', '8']
ompcores = ['2', '4']
MPI=False
MPI_executables = ["send", "bcast"]
Hybrid=False

def main():
    testFiles = [f for f in os.listdir(testFolder) if os.path.isfile(os.path.join(testFolder, f))]
    for cores in ncores:
        for testFile in testFiles:      
            if MPI:
                for executable in MPI_executables: 
                    os.system("./run_mpi.sh {0} {1} {2}".format(executable, testFile, cores))
            elif Hybrid:
                for ompcore in ompcores: 
                    os.system("./run_hybrid.sh {0} {1} {2}".format(testFile, cores, ompcore))
            else:
                os.system("./run.sh {0} {1}".format(testFile, cores))

if __name__ == '__main__':
    if len(sys.argv) > 1:
        if sys.argv[1] == "MPI":
            MPI = True
        elif sys.argv[1] == "Hybrid":
            Hybrid=True
    main()