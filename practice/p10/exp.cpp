#include <iostream>
#include <cstdlib>
#include "mpi.h"

double fact(int n) {
    if (n) {
        return n * fact(n - 1);
    } else {
        return 1;
    }
}

int main(int argc, char *argv[])
{
    int n, n_proc, rank;
    double start_time, stop_time;
    long double res, sum = 0;

    if (argc != 2) {
        std::cout << "Введите n - количество членов ряда Тейлора как аргумент командной строки" << std::endl;
    }
    n = atoi(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &n_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        start_time = MPI_Wtime();
    }
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    for (int i = rank; i <= n; i += n_proc) {
        sum += 1 / fact(i);
    }

    MPI_Reduce(&sum, &res, 1, MPI_LONG_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "exp(1) = " << res << std::endl;
        stop_time = MPI_Wtime();
        std::cout << "Calculations took " << (stop_time - start_time) * 1000 << " ms" << std::endl;
    }

    MPI_Finalize();
    return 0;
}