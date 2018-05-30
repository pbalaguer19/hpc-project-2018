#!/bin/bash
qsub -S /bin/bash -V -v TESTFILE=$1,CORES=$2,OMPCORES=$3 -N OptimalFenceHybrid_$1_$2_$3 -wd $HOME/results/OptimalFence/Hybrid -o $HOME/results/OptimalFence/Hybrid -e $HOME/results/OptimalFence/Hybrid -pe mpich-smp $(($2 * 4)) ./hybrid_helper.sh