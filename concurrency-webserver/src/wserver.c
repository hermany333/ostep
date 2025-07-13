#include "fdbuf.h"
#include "io_helper.h"
#include "thpool.h"
#include <stdio.h>
#include <stdlib.h>

char default_root[] = ".";

//
// ./wserver [-d <basedir>] [-p <portnum>]
//
int main(int argc, char *argv[]) {
  int c;
  char *root_dir = default_root;
  int port = 10000;
  int num_threads = 1; // Default for -t is 1
  int buffer_num = 1;  // default -b 1

  while ((c = getopt(argc, argv, "d:p:t:b:")) != -1)
    switch (c) {
    case 'd':
      root_dir = optarg;
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 't':
      num_threads = atoi(optarg);
      break;
    case 'b':
      buffer_num = atoi(optarg);
      break;
    default:
      fprintf(
          stderr,
          "usage: wserver [-d basedir] [-t threads] [-p port] [-b buffers]\n");
      exit(1);
    }

  // run out of this directory
  chdir_or_die(root_dir);

  // init fdbuffer;
  fdbuf_t fdbuffer = init_fdbuf(buffer_num);
  // init thpool;
  threadpool_t threadpool = init_threadpool(num_threads, fdbuffer);
  // now, get to work
  int listen_fd = open_listen_fd_or_die(port);
  while (1) {
    struct sockaddr_in client_addr;
    int client_len = sizeof(client_addr);
    int conn_fd = accept_or_die(listen_fd, (sockaddr_t *)&client_addr,
                                (socklen_t *)&client_len);

    // main produces fds and fills fdbuffer with them;
    fdbuffer_lock(fdbuffer);
    while (get_fdbuffer_count(fdbuffer) ==
           buffer_num) // while count == max wait on cond empty
      fdbuffer_wait_empty(fdbuffer);
    fdbuffer_fill(fdbuffer, conn_fd);
    fdbuffer_signal_fill(fdbuffer);
    fdbuffer_unlock(fdbuffer);
  }

  // clean up
  destroy_fdbuf(fdbuffer);
  destroy_threadpool(threadpool);

  return 0;
}
