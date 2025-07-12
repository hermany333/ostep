#include "thpool.h"
#include "io_helper.h"
#include "request.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct thread_ {
  int id;
  pthread_t pthread;
};

struct threadpool_ {
  struct thread_ *threads;
  int num_threads;
};

void *worker_thread(void *);
void threadpool_destroy(threadpool_t, int);

threadpool_t init_threadpool(int num_threads, fdbuf_t fdbuffer) {
  int id = 0;
  threadpool_t threadpool = malloc(sizeof(*threadpool));

  if (threadpool == NULL)
    return NULL;

  threadpool->num_threads = num_threads;
  threadpool->threads = malloc(sizeof(thread_t) * num_threads);

  if (threadpool->threads == NULL)
    return NULL;

  for (int i = 0; i < num_threads; i++) {
    threadpool->threads[i].id = id++;
    pthread_create(&threadpool->threads[i].pthread, NULL, worker_thread,
                   fdbuffer);
  }

  return threadpool;
}

void destroy_threadpool(threadpool_t threadpool) {
  free(threadpool->threads);
  free(threadpool);
}

void *worker_thread(void *args) {
  fdbuf_t fdbuffer = (fdbuf_t)args;

  while (1) {
    fdbuffer_lock(fdbuffer);
    while (get_fdbuffer_count(fdbuffer) == 0)
      fdbuffer_wait_fill(fdbuffer);
    int fd = fdbuffer_get(fdbuffer);

    fdbuffer_signal_empty(fdbuffer);
    fdbuffer_unlock(fdbuffer);
    printf("[Worker %lu] Handling connection on fd %d\n",
           (unsigned long)pthread_self(), fd);
    request_handle(fd);
    close_or_die(fd);
  }
};
