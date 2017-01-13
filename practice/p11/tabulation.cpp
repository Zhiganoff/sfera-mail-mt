#include <iostream>
#include <cstdlib>
// #include <string.h> 
#include <cmath>
#include <ctime>
#include <vector>
#include <openacc.h>

#define N_STEPS 10000000
#define START_POINT -10
#define END_POINT 10

using std::cout;
using std::endl;

double test_func(double x) {
    return x * log(1 + x) + sin(x) * cos(3 * x) * exp(x / 5) / (4 * x * x + 3 * x - 8);
}

int main()
{
    /* code */
    cout << "Табуляция функции x * ln(1 + x) + sin(x) * cos(3 * x) * exp(x / 5) / (4 * x * x + 3 * x - 8)" << endl;
    cout << "Отрезок [" << START_POINT << "; " << END_POINT << "]" << endl;
    cout << "Количество шагов " << N_STEPS << endl;

    double x = START_POINT;
    double delta_x = (END_POINT - START_POINT) / (double) N_STEPS;

    clock_t start_time = clock();
    double *value_array = (double *) calloc(N_STEPS + 1, sizeof(double));

    #pragma acc kernels
    for (int idx = 0; idx <= N_STEPS; idx++) {
        value_array[idx] = test_func(x + delta_x * idx);
    }

    cout << "Время: " << (double) (clock() - start_time) / CLOCKS_PER_SEC << " секунд" << endl;
    return 0;
}
