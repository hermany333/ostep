#include "fdbuf.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct fdbuf_ {
  int *fds;
  int max;
  int count;
  int *fill_ptr;
  int *use_ptr;
  pthread_mutex_t mutex;
  pthread_cond_t empty;
  pthread_cond_t fill;
};

fdbuf_t init_fdbuf(int buf_num) {
  fdbuf_t fdbuffer = malloc(sizeof(*fdbuffer));

  fdbuffer->fds = malloc(sizeof(int) * buf_num);
  fdbuffer->max = buf_num;
  fdbuffer->count = 0;
  fdbuffer->fill_ptr = fdbuffer->use_ptr = fdbuffer->fds;
  pthread_mutex_init(&fdbuffer->mutex, NULL);
  pthread_cond_init(&fdbuffer->empty, NULL);
  pthread_cond_init(&fdbuffer->fill, NULL);

  return fdbuffer;
}

void destroy_fdbuf(fdbuf_t fdbuffer) {
  pthread_mutex_destroy(&fdbuffer->mutex);
  pthread_cond_destroy(&fdbuffer->fill);
  pthread_cond_destroy(&fdbuffer->empty);

  free(fdbuffer->fds);
  free(fdbuffer);
}

int fdbuffer_get(fdbuf_t fdbuffer) {
  int tmp = *(fdbuffer->use_ptr);
  fdbuffer->use_ptr++;
  if (fdbuffer->use_ptr >= fdbuffer->fds + fdbuffer->max) {
    fdbuffer->use_ptr = fdbuffer->fds;
  }

  fdbuffer->count--;

  return tmp;
}

void fdbuffer_fill(fdbuf_t fdbuffer, int fd) {
  *fdbuffer->fill_ptr = fd;
  fdbuffer->fill_ptr++;

  if (fdbuffer->fill_ptr >= fdbuffer->fds + fdbuffer->max) {
    fdbuffer->fill_ptr = fdbuffer->fds;
  }

  fdbuffer->count++;
}

int get_fdbuffer_count(fdbuf_t fdbuffer) { return fdbuffer->count; }

void fdbuffer_lock(fdbuf_t fdbuffer) { pthread_mutex_lock(&fdbuffer->mutex); }

void fdbuffer_unlock(fdbuf_t fdbuffer) {
  pthread_mutex_unlock(&fdbuffer->mutex);
}

void fdbuffer_wait_fill(fdbuf_t fdbuffer) {
  pthread_cond_wait(&fdbuffer->fill, &fdbuffer->mutex);
}

void fdbuffer_wait_empty(fdbuf_t fdbuffer) {
  pthread_cond_wait(&fdbuffer->empty, &fdbuffer->mutex);
}

void fdbuffer_signal_fill(fdbuf_t fdbuffer) {
  pthread_cond_signal(&fdbuffer->fill);
}

void fdbuffer_signal_empty(fdbuf_t fdbuffer) {
  pthread_cond_signal(&fdbuffer->empty);
}
