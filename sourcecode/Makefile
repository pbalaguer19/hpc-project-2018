all:
	gcc -std=c99 optimalfence-omp.c -o OptimalFence -fopenmp -lm
exec:
	./OptimalFence ../testbed/test_trees6.txt output.res
clear:
	rm .res OptimalFence
