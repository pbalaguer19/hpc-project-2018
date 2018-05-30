#!/bin/bash
qsub -S /bin/bash -V -pe smp 4 -wd $HOME/results/OptimalFence/OMP -N OptimalFenceOMP -v OMP_NUM_THREADS=$2 -b y $HOME/hpc-project-2018/OptimalFence $HOME/hpc-project-2018/testbed/$1
