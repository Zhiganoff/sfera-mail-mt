gcc merge_sort_openmp.c -fopenmp -o mergesort
./mergesort > test.out
cat test.out