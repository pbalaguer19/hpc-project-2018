# HPC Project

## Authors
- Pau Balaguer
- Javier Bonet

## Structure
```
hpc-project-2018
  |
  |-results
  |     |
  |     |-OMP
  |
  |-sourcecode
  |
  |_testbed
```
- **Results:** folder with the output results. The filenames is build following:
```
OptimalFenceOMP_INPUTFILENAME_NUMBEROFTHREADS.oID
```
where `INPUTFILENAME`is the input file with the trees, `NUMBEROFTHREADS` is the number of threads and `ID` is the process identifier from the Moore's Cluster.

- **Sourcecode:** folder with the problem code and its Makefile.

- **Testbed:** folder with the different input files. Each line in the inut file represents a tree (excluding the first one that is general information).

## Compilation

```
gcc -std=c99 optimalfence-omp.c -o OptimalFence -fopenmp -lm
```

## Execution

```
./OptimalFence INPUT_FILE OUTPUT_FILE
```

## Other comments

- The `executor.py` script is created in order to execute all the combinations in the Moore's Cluster.

- The `run.sh` script is created submit a batch job to Sun Grid Engine.
