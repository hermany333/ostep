#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: ./memory-user <megabytes>\n");
    return EXIT_FAILURE;
  }

  size_t bytes = atoi(argv[1]) * 1024 * 1024;

  char *arr = malloc(bytes);

  if (arr == NULL) {
    fprintf(stderr, "malloc call failed\n");
  }

  int i = 0;
  printf("pid: %d\n", getpid());
  while (1) {
    arr[i++ % bytes] = 0;
  }

  return EXIT_SUCCESS;
}
