#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void decompress(const char *file_name) {
  int count;
  char ch;

  FILE *fp = fopen(file_name, "rb");

  if (fp == NULL) {
    fprintf(stdout, "wzip: cannot open file\n");
    exit(1);
  }

  while (fread(&count, sizeof(int), 1, fp) == 1 &&
         fread(&ch, sizeof(char), 1, fp) == 1) {
    for (int i = 0; i < count; i++) {
      fputc(ch, stdout);
    }
  }

  if (ferror(fp)) {
    fprintf(stdout, "wzip: error reading file %s\n", file_name);
    fclose(fp);
  }

  fclose(fp);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stdout, "wunzip: file1 [file2 ...]\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 1; i < argc; i++) {
    decompress(argv[i]);
  }

  return EXIT_SUCCESS;
}
