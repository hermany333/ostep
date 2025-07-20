#include "jobbuf.h"
#include <stdio.h>
#include <stdlib.h>

jobbuf_t init_jobbuf(int jobcount) {
  jobbuf_t jobbuffer = malloc(sizeof(*jobbuffer));

  jobbuffer->jobs = malloc(sizeof(job_t) * jobcount);
  jobbuffer->max = jobcount;
  jobbuffer->count = 0;
  jobbuffer->fill_ptr = jobbuffer->use_ptr = 0;
  pthread_mutex_init(&jobbuffer->mutex, NULL);
  pthread_cond_init(&jobbuffer->empty, NULL);
  pthread_cond_init(&jobbuffer->fill, NULL);

  return jobbuffer;
}

void destroy_jobbuf(jobbuf_t jobbuffer) {
  pthread_mutex_destroy(&jobbuffer->mutex);
  pthread_cond_destroy(&jobbuffer->fill);
  pthread_cond_destroy(&jobbuffer->empty);

  free(jobbuffer->jobs);
  free(jobbuffer);
}

job_t jobbuffer_get(jobbuf_t jobbuffer) {
  job_t tmp = jobbuffer->jobs[jobbuffer->use_ptr];
  jobbuffer->use_ptr = (jobbuffer->use_ptr + 1) % jobbuffer->max;
  jobbuffer->count--;
  return tmp;
}

void jobbuffer_fill(jobbuf_t jobbuffer, job_t job) {
  jobbuffer->jobs[jobbuffer->fill_ptr] = job;
  jobbuffer->fill_ptr = (jobbuffer->fill_ptr + 1) % jobbuffer->max;
  jobbuffer->count++;
}

int get_jobbuffer_count(jobbuf_t jobbuffer) { return jobbuffer->count; }

void jobbuffer_lock(jobbuf_t jobbuffer) {
  pthread_mutex_lock(&jobbuffer->mutex);
}

void jobbuffer_unlock(jobbuf_t jobbuffer) {
  pthread_mutex_unlock(&jobbuffer->mutex);
}

void jobbuffer_wait_fill(jobbuf_t jobbuffer) {
  pthread_cond_wait(&jobbuffer->fill, &jobbuffer->mutex);
}

void jobbuffer_wait_empty(jobbuf_t jobbuffer) {
  pthread_cond_wait(&jobbuffer->empty, &jobbuffer->mutex);
}

void jobbuffer_signal_fill(jobbuf_t jobbuffer) {
  pthread_cond_signal(&jobbuffer->fill);
}

void jobbuffer_signal_empty(jobbuf_t jobbuffer) {
  pthread_cond_signal(&jobbuffer->empty);
}
