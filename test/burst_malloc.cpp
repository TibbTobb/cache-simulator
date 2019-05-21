#include <assert.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>

static int
do_some_work(int arg)
{
    static const int iters = 100000;
    auto *val = new double; // libc malloc is called inside new
    auto **vals = (double **)calloc(iters, sizeof(double *));
    *val = arg;

    for (int i = 0; i < iters; ++i) {
        vals[i] = (double *)malloc(sizeof(double));
        *vals[i] = sin(*val);
        *val += *vals[i];
    }
    for (int i = 0; i < iters; i++) {
        *val += *vals[i];
    }
    for (int i = 0; i < iters; i++) {
        free(vals[i]);
    }
    free(vals);
    double temp = *val;
    delete val; // libc free is called inside delete
    return (temp > 0);
}

int
main(int argc, const char *argv[]) {
    for (int i = 0; i < 3; i++) {

        if (do_some_work(i * 2) < 0)
            std::cerr << "error in computation\n";

        return 0;
    }
}