#!/bin/bash
MPICH_MACHINES=$TMPDIR/mpich_machines
cat $PE_HOSTFILE | awk '{print $1":1"}' > $MPICH_MACHINES

## In this line you have to write the command that will execute your application.
time mpiexec -f $MPICH_MACHINES -n $NHOSTS -genv OMP_NUM_THREADS $OMPCORES $HOME/hpc-project-2018/sourcecode/OptimalFenceHybrid $HOME/hpc-project-2018/testbed/$TESTFILE $HOME/results/OptimalFence/Hybrid/results_$TESTFILE_$NHOSTS_$OMPCORES.res

rm -rf $MPICH_MACHINES
