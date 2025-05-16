#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void grep(FILE *fp, const char *searchterm) {
  size_t size = 0;
  ssize_t len = 0;
  char *line = NULL;

  while ((len = getline(&line, &size, fp)) != -1) {
    if (strstr(line, searchterm) != NULL)
      fprintf(stdout, "%s", line);
  }

  free(line);
  return;
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stdout, "wgrep: searchterm [file ...]\n");
    exit(1);
  }

  char *searchterm = argv[1];

  if (argc == 2) {
    grep(stdin, searchterm);
    return EXIT_SUCCESS;
  }

  for (int i = 2; i < argc; i++) {
    FILE *fp = fopen(argv[i], "r");

    if (fp == NULL) {
      fprintf(stdout, "wgrep: cannot open file\n");
      exit(1);
    }

    grep(fp, searchterm);
    fclose(fp);
  }

  return EXIT_SUCCESS;
}
