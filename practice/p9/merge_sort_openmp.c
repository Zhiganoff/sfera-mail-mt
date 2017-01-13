#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "omp.h"

#define N_ELEMS 1000000
#define MERGE_TRESHOLD 1000

void generate_array(int *x, int n) {
    int idx;
    for (idx = 0; idx < n; idx++) {
        x[idx] = rand();
    }
}

void merge(int *x, int n, int *buf) {
    int i1 = 0;
    int i2 = n / 2;
    int j = 0;

    while (i1 < n / 2 && i2 < n) {
        if (x[i1] < x[i2]) {
            buf[j] = x[i1];
            j++;
            i1++;
        } else {
            buf[j] = x[i2];
            j++;
            i2++;
        }
    }

    while (i1 < n / 2) {
        buf[j] = x[i1];
        j++;
        i1++;
    }

    while (i2 < n) {
        buf[j] = x[i2];
        j++;
        i2++;
    }

    memcpy(x, buf, n * sizeof(int));
}

void merge_sort(int *x, int n, int *buf) {
    if (n < 2) return;

    if (n > MERGE_TRESHOLD) {
        // Parallel sorting
        #pragma omp task firstprivate(x, n, buf)
        merge_sort(x, n/2, buf);

        #pragma omp task firstprivate(x, n, buf)
        merge_sort(x + n / 2, n - n / 2, buf + n / 2);

        #pragma omp taskwait
    } else {
        // Sorting on one thread
        merge_sort(x, n/2, buf);
        merge_sort(x + n / 2, n - n / 2, buf + n / 2);
    }
    merge(x, n, buf);
}

void run_test(int n_threads, int *data, int *buf) {
    double start_time, stop_time;
    omp_set_num_threads(n_threads);
    generate_array(data, N_ELEMS);
    start_time = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp single
        merge_sort(data, N_ELEMS, buf);
    }
    stop_time = omp_get_wtime();
    printf("Time: %g\n", stop_time - start_time);
}

int main() {
    int data[N_ELEMS], buf[N_ELEMS];

    printf("Однопоточная сортировка слиянием\n");
    run_test(1, data, buf);

    printf("Распараллеленная на 4 потока сортировка слиянием:\n");
    run_test(4, data, buf);

    return 0;
}