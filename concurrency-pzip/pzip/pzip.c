#include "jobbuf.h"
#include <bits/pthreadtypes.h>
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

#define CHUNK_SIZE 1024

typedef struct compressed_chunk {
  char *compressed;
  size_t size;
} compressed_chunk;

typedef struct worker_args {
  jobbuf_t jobbuffer;
  compressed_chunk *compressed_array;
} worker_args;

char *compress(char *, size_t, size_t *);
void compress_consolidate(compressed_chunk *compressed_array, size_t length);

void *worker(void *arg) {
  worker_args *args = (worker_args *)arg;
  jobbuf_t jobbuffer = args->jobbuffer;
  compressed_chunk *compressed_arr = args->compressed_array;

  while (1) {
    jobbuffer_lock(jobbuffer);

    while (get_jobbuffer_count(jobbuffer) == 0)
      jobbuffer_wait_fill(jobbuffer);

    job_t job = jobbuffer_get(jobbuffer);

    jobbuffer_signal_empty(jobbuffer);
    jobbuffer_unlock(jobbuffer);
    if (job->kill) {
      free(job);
      break;
    }

    size_t out_size = 0;

    char *result = compress(job->chunk_start, job->chunk_size, &out_size);
    compressed_arr[job->chunk_id].compressed = result;
    compressed_arr[job->chunk_id].size = out_size;

    free(job);
  }

  return NULL;
}

void producer(char *file, int filesize, jobbuf_t jobbuffer, int num_threads) {
  size_t offset = 0;
  int chunk_id = 0;

  while (offset < filesize) {
    size_t remaining = filesize - offset;
    size_t this_chunk_size = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

    job_t job = malloc(sizeof(struct job_));
    job->chunk_id = chunk_id;
    job->chunk_size = this_chunk_size;
    job->chunk_start = file + offset;
    job->kill = NULL;

    jobbuffer_lock(jobbuffer);

    while (get_jobbuffer_count(jobbuffer) == jobbuffer->max)
      jobbuffer_wait_empty(jobbuffer);
    jobbuffer_fill(jobbuffer, job);

    jobbuffer_signal_fill(jobbuffer);
    jobbuffer_unlock(jobbuffer);

    offset += this_chunk_size;
    chunk_id++;
  }

  // Poison pill strategy
  for (int i = 0; i < num_threads; i++) {
    job_t kill_job = malloc(sizeof(struct job_));
    kill_job->kill = (void *)1; // anything non-NULL signals to exit

    jobbuffer_lock(jobbuffer);

    while (get_jobbuffer_count(jobbuffer) == jobbuffer->max)
      jobbuffer_wait_empty(jobbuffer);

    jobbuffer_fill(jobbuffer, kill_job);

    jobbuffer_signal_fill(jobbuffer);
    jobbuffer_unlock(jobbuffer);
  }
}

int main(int argc, char *argv[]) {

  if (argc == 1) {
    fprintf(stdout, "pzip: file1 [file2 ...]\n");
    exit(EXIT_FAILURE);
  }

  /**
   === OPEN FILE AND MMAP TO IN-MEMORY LOCATION ===
   **/
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(stdout, "pzip: cannot open file\n");
    exit(1);
  }
  struct stat filestat;
  if (fstat(fd, &filestat) != 0) {
    perror("stat failed");
    exit(1);
  }
  int filesize = (int)filestat.st_size;
  char *file = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);

  // init jobbuffer bounded queue with 16 slots

  /**
   === FIND NUMBER OF THREADS AND INIT SAME NUMBER OF WORKER THREADS CREATE FOR
   DIFFERENT ARGS, ALSO INIT jobbuffer bounded queue n = 16
   **/
  int num_threads = get_nprocs();
  jobbuf_t jobbuffer = init_jobbuf(16);

  pthread_t threads[num_threads];
  worker_args worker_args[num_threads];

  size_t num_chunks = (filesize + CHUNK_SIZE - 1) / CHUNK_SIZE;
  compressed_chunk compressed_arr[num_chunks];

  for (int i = 0; i < num_threads; i++) {
    worker_args[i].jobbuffer = jobbuffer;
    worker_args[i].compressed_array = compressed_arr;
  }

  // create threads
  for (int i = 0; i < num_threads; i++) {
    pthread_create(&threads[i], NULL, worker, &worker_args[i]);
  }

  producer(file, filesize, jobbuffer, num_threads);
  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  compress_consolidate(compressed_arr, num_chunks);
  destroy_jobbuf(jobbuffer);

  return EXIT_SUCCESS;
}

char *compress(char *start, size_t len, size_t *out_size) {
  size_t capacity = 128;
  size_t length = 0;
  char *output = malloc(capacity);

  if (!output)
    return NULL;

  if (len == 0)
    return output;

  char prev = start[0];
  int count = 1;

  for (size_t i = 1; i < len; i++) {
    if (start[i] == prev) {
      count++;
    } else {
      char buf[32];
      int n = snprintf(buf, sizeof(buf), "%d%c", count, prev);

      if (length + n >= capacity) {
        capacity *= 2;
        char *new_output = realloc(output, capacity);
        if (!new_output) {
          free(output);
          return NULL;
        }
        output = new_output;
      }

      memcpy(output + length, buf, n);
      length += n;

      prev = start[i];
      count = 1;
    }
  }

  char buf[32];
  int n = snprintf(buf, sizeof(buf), "%d%c", count, prev);
  if (length + n >= capacity) {
    capacity += n;
    char *new_output = realloc(output, capacity);
    if (!new_output) {
      free(output);
      return NULL;
    }
    output = new_output;
  }
  memcpy(output + length, buf, n);
  length += n;

  if (out_size)
    *out_size = length;

  return output;
}

void compress_consolidate(compressed_chunk *compressed_array, size_t length) {
  int last_count = 0;
  char last_ch = '\0';
  int initialized = 0;

  for (size_t i = 0; i < length; i++) {
    const char *p = compressed_array[i].compressed;

    while (*p) {
      int num;
      char ch;
      if (sscanf(p, "%d%c", &num, &ch) != 2)
        break;

      char buf[32] = {0};
      snprintf(buf, sizeof(buf), "%d", num);
      size_t consumed = strlen(buf) + 1;
      p += consumed;

      if (!initialized) {
        last_count = num;
        last_ch = ch;
        initialized = 1;
      } else if (ch == last_ch) {
        last_count += num;
      } else {
        fwrite(&last_count, sizeof(int), 1, stdout);
        fwrite(&last_ch, sizeof(char), 1, stdout);
        last_count = num;
        last_ch = ch;
      }
    }
  }

  if (initialized) {
    fwrite(&last_count, sizeof(int), 1, stdout);
    fwrite(&last_ch, sizeof(char), 1, stdout);
  }
}
