#include <stdlib.h>
int main(int argc, char *argv[]) {
  int *arr = malloc(100 * sizeof(int));

  arr[100] = 0;

  free((arr + 50));

  return EXIT_SUCCESS;
}
