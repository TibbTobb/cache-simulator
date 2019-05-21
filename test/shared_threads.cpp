#include <assert.h>
#include <iostream>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <pthread.h>


struct args_struct
{
    int idx;
    int *vals;
};

static const int num_threads = 2;
static bool finished[num_threads];
static pthread_cond_t worker_finnished;
static pthread_mutex_t mutex;

static int iters = 1000;
static int num_writes = 0;
static int num_reads = 0;



void write(int *vals, int arg) {
    for (int i = 0; i < iters; ++i) {
        vals[i] = arg*i;

    }
}

void read(int *vals) {
    int total = 0;
    for(int i=0; i< iters; i++) {
        total += vals[i];
    }
    printf("%i\n", total);
}

void *thread_func(void *arguments) {
    auto  *args = (struct args_struct*) arguments;
    static const int iters = 1000;
    if((args->idx % 2) == 0) {
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < iters; i++) {
            write(args->vals, i);
            num_writes++;
            printf("write: %i\n", args->idx);
            pthread_cond_signal(&worker_finnished);
            pthread_cond_wait(&worker_finnished, &mutex);
        }
    } else {
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < iters; i++) {
            read(args->vals);
            num_reads++;
            printf("read: %i\n", args->idx);
            pthread_cond_signal(&worker_finnished);
            pthread_cond_wait(&worker_finnished, &mutex);
        }
    }
    pthread_cond_signal(&worker_finnished);
    pthread_mutex_unlock(&mutex);
    finished[args->idx] = true;
    return 0;
}


int
main(int argc, const char *argv[])
{

    auto *vals = (int *)malloc(iters*sizeof(int));

    pthread_t thread[num_threads];

    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&worker_finnished, nullptr);

    for (unsigned long i = 0; i < num_threads; i++) {
        auto *args = static_cast<args_struct *>(malloc(sizeof(struct args_struct)));
        args->idx = static_cast<int>(i);
        args->vals = vals;
        pthread_create(&thread[i], nullptr, thread_func, (void *)args);
    }
    for (pthread_t t : thread) {
        pthread_join(t, nullptr);
    }
    for (unsigned long i=0; i < num_threads; i++) {
        if (!finished[i])
            std::cerr << "thread " << i << " failed to finish\n";
    }
    printf("all done\n");
    pthread_cond_destroy(&worker_finnished);
    pthread_mutex_destroy(&mutex);
    return 0;
}