#ifndef THPOOl_H
#define THPOOl_H

#include "fdbuf.h"

typedef struct threadpool_ *threadpool_t;
typedef struct thread_ *thread_t;

threadpool_t init_threadpool(int, fdbuf_t fdbuffer);
void destroy_threadpool(threadpool_t threadpool);

#endif // !THPOOl_H
