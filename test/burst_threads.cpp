#include <assert.h>
#include <iostream>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <pthread.h>

static const int num_threads = 4;
static const int burst_owner = 0;
static bool finished[num_threads];
static int not_finnished = num_threads;
static pthread_cond_t burst_owner_finished;
static pthread_cond_t worker_finnished;
static pthread_mutex_t mutex;

static int
do_some_work(int i)
{
    static int iters = 512;
    //static int iters = 32;
    auto val = (double)i;
    for (int i = 0; i < iters; ++i) {
        val += sin(val);
    }
    return (val > 0);
}
void *thread_func(void *arg) {
    auto idx = (unsigned int) (uintptr_t) arg;
    static const int iters = 16;
    for (int i = 0; i < iters; i++) {
        if(do_some_work(i) < 0) {
            std::cerr << "error in computation\n";
        }
    }

    if (idx == burst_owner) {
        pthread_mutex_lock(&mutex);
        not_finnished--;
        while(not_finnished > 0) {
            pthread_cond_wait(&worker_finnished, &mutex);
        }
        pthread_cond_broadcast(&burst_owner_finished);
        pthread_mutex_unlock(&mutex);
    } else {
        //Avoid having < 1 thread per core in the output.
        pthread_mutex_lock(&mutex);
        not_finnished--;
        pthread_cond_signal(&worker_finnished);
        pthread_cond_wait(&burst_owner_finished, &mutex);
        pthread_mutex_unlock(&mutex);
    }

    finished[idx] = true;
    return 0;
}

int
main(int argc, const char *argv[])
{
    pthread_t thread[num_threads];

    /* While the start/stop thread only runs 4 iters, the other threads end up
     * running more and their trace files get up to 65MB or more, with the
     * merged result several GB's: too much for a test.  We thus cap each thread.
     */
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&burst_owner_finished, nullptr);

    for (unsigned long i = 0; i < num_threads; i++) {
        pthread_create(&thread[i], nullptr, thread_func, (void *)i);
    }
    for (pthread_t t : thread) {
        pthread_join(t, nullptr);
    }
    for (unsigned long i=0; i < num_threads; i++) {
        if (!finished[i])
            std::cerr << "thread " << i << " failed to finish\n";
    }
    std::cerr << "all done\n";
    pthread_cond_destroy(&burst_owner_finished);
    pthread_mutex_destroy(&mutex);
    return 0;
}