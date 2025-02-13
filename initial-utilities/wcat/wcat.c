#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  size_t size = 0;
  ssize_t len = 0;
  char *line = NULL;

  if (argc == 1)
    return 0;

  for (int i = 1; i < argc; i++) {
    FILE *fp = fopen(argv[i], "r");

    if (fp == NULL) {
      fprintf(stdout, "wcat: cannot open file\n");
      exit(1);
    }

    while ((len = getline(&line, &size, fp)) != -1) {
      fprintf(stdout, "%s", line);
    }
  }

  free(line);

  return EXIT_SUCCESS;
}
