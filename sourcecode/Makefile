all:
	gcc -std=c99 optimalfence-omp.c -o OptimalFence -fopenmp -lm
	mpicc optimalfence-mpi_send.c -o OptimalFenceMPI_send -lm
	mpicc optimalfence-mpi_bcast.c -o OptimalFenceMPI_bcast -lm
runMPI:
	python executor.py MPI
clear:
	rm .res OptimalFence OptimalFenceMPI*
