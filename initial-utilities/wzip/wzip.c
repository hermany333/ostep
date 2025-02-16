#include <stdio.h>
#include <stdlib.h>

void compress(const char *file_name, char *prev, int *count, int *first_file) {
  char ch;

  FILE *fp = fopen(file_name, "r");

  if (fp == NULL) {
    fprintf(stdout, "wzip: cannot open file\n");
    exit(1);
  }

  if (*first_file) {
    if ((*prev = fgetc(fp)) == EOF) {
      fclose(fp);
      return;
    }
    *count = 1;
    *first_file = 0;
  }

  while ((ch = fgetc(fp)) != EOF) {
    if (ch == *prev) {
      *count = *count + 1;
    } else {

      fwrite(count, sizeof(int), 1, stdout);
      fwrite(prev, sizeof(char), 1, stdout);
      fflush(stdout);

      *prev = ch;
      *count = 1;
    }
  }

  fclose(fp);
}

int main(int argc, char *argv[]) {
  int count = 0, first_file = 1;

  char prev = '\0'; // Last character

  if (argc == 1) {
    fprintf(stdout, "wzip: file1 [file2 ...]\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 1; i < argc; i++) {
    compress(argv[i], &prev, &count, &first_file);
  }

  if (count > 0) {
    fwrite(&count, sizeof(int), 1, stdout);
    fwrite(&prev, sizeof(char), 1, stdout);
    fflush(stdout);
  }

  return EXIT_SUCCESS;
}
