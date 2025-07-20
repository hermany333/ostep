#ifndef JOBBUF_H
#define JOBBUF_H

#include <pthread.h>
#include <stdbool.h>

typedef struct job_ *job_t;
typedef struct jobbuf_ *jobbuf_t;

struct job_ {
  char *chunk_start;
  size_t chunk_size; // 1024
  int chunk_id;
  bool kill;
};

struct jobbuf_ {
  job_t *jobs;
  int max;
  int count;
  int fill_ptr;
  int use_ptr;
  pthread_mutex_t mutex;
  pthread_cond_t empty;
  pthread_cond_t fill;
};

jobbuf_t init_jobbuf(int);
void destroy_jobbuf(jobbuf_t);

int get_jobbuffer_count(jobbuf_t);

void jobbuffer_lock(jobbuf_t jobbuffer);
void jobbuffer_unlock(jobbuf_t jobbuffer);

void jobbuffer_wait_fill(jobbuf_t jobbuffer);
void jobbuffer_wait_empty(jobbuf_t jobbuffer);
void jobbuffer_signal_fill(jobbuf_t jobbuffer);
void jobbuffer_signal_empty(jobbuf_t jobbuffer);

job_t jobbuffer_get(jobbuf_t jobbuffer);
void jobbuffer_fill(jobbuf_t jobbuffer, job_t job);

#endif
