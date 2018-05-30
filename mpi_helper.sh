#!/bin/bash
MPICH_MACHINES=$TMPDIR/mpich_machines
cat $PE_HOSTFILE | awk '{print $1":"$2}' > $MPICH_MACHINES

## In this line you have to write the command that will execute your application.
time mpiexec -f $MPICH_MACHINES -n $NSLOTS $HOME/hpc-project-2018/sourcecode/OptimalFenceMPI_$SUFFIX $HOME/hpc-project-2018/testbed/$TESTFILE $HOME/results/OptimalFence/MPI/results_$SUFFIX$TESTFILE$NSLOTS.res

rm -rf $MPICH_MACHINES