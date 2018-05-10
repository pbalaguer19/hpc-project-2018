# coding = utf8

import os
import sys 

testFolder = "testbed"
ncores = ['1', '2', '4', '8', '16', '32']

MPI=False
MPI_executables = ["send", "bcast"]

def main():
    testFiles = [f for f in os.listdir(testFolder) if os.path.isfile(os.path.join(testFolder, f))]
    for cores in ncores:
        for testFile in testFiles:      
            if MPI:
                for executable in MPI_executables: 
                    os.system("./run_mpi.sh {0} {1} {2}".format(executable, testFile, cores))
            else:
                os.system("./run.sh {0} {1}".format(testFile, cores))

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == "MPI":
        MPI = True
    main()