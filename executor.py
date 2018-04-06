# coding = utf8

import os

testFolder = "testbed"
ncores = ['1', '2', '4']

def main():
    testFiles = [f for f in os.listdir(testFolder) if os.path.isfile(os.path.join(testFolder, f))]
    for cores in ncores:
        for testFile in testFiles:            
            os.system("./run.sh {0} {1}".format(testFile, cores))

if __name__ == '__main__':
    main()