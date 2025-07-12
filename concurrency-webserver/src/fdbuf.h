#ifndef FDBUF_H
#define FDBUF_H

#include <pthread.h>

typedef struct fdbuf_ *fdbuf_t;

fdbuf_t init_fdbuf(int);
void destroy_fdbuf(fdbuf_t);
int get_fdbuffer_count(fdbuf_t);

void fdbuffer_lock(fdbuf_t fdbuffer);
void fdbuffer_unlock(fdbuf_t fdbuffer);

void fdbuffer_wait_fill(fdbuf_t fdbuffer);
void fdbuffer_wait_empty(fdbuf_t fdbuffer);
void fdbuffer_signal_fill(fdbuf_t fdbuffer);
void fdbuffer_signal_empty(fdbuf_t fdbuffer);

int fdbuffer_get(fdbuf_t fdbuffer);
void fdbuffer_fill(fdbuf_t fdbuffer, int fd);

#endif
