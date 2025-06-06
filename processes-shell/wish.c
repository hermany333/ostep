#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define PATHS 16
#define ARGS 16

void null_paths(char *paths[]);
void free_paths(char *paths[]);

int main(int argc, char *argv[]) {
  char *line = NULL;
  size_t len = 0;
  char *paths[PATHS] = {NULL};

  char error_message[30] = "An error has occurred\n";

  while (true) {
    // CLI display
    fprintf(stdout, "wish> ");
    fflush(stdout);

    // Get user input
    size_t nread;
    nread = getline(&line, &len, stdin);

    if (nread == -1) {
      perror("getline");
    }

    line[strcspn(line, "\n")] = '\0';

    // Process built-in commands ["exit", "cd", "path", other binaries via fork
    // and exec]
    char *line_cpy = line;
    char *cmd = strsep(&line_cpy, " ");

    if (strcmp(cmd, "exit") == 0) {
      break;

    } else if (strcmp(cmd, "cd") == 0) {

      char *cd_arg = strsep(&line_cpy, " ");
      if (cd_arg == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
      }

      int res = chdir(cd_arg);

      if (res == -1) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
      }

    } else if (strcmp(cmd, "path") == 0) {
      // null out path array if this is not the first time in the same session
      free_paths(paths);

      char *path_arg;
      int i = 0;
      while ((path_arg = strsep(&line_cpy, " ")) != NULL) {
        printf("user sets PATH variable: %s \n", path_arg);
        paths[i++] = strdup(path_arg);
      }
      // i = 0;
      // while (paths[i] != NULL) {
      //   printf("%s\n", paths[i++]);
      // }
    } else {
      // binaries through exec
      int rc = fork();

      if (rc < 0) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);

      } else if (rc == 0) {
        char *args[ARGS];
        int argc = 0;
        char exec_path[512];
        char *token = cmd;
        bool found = false;

        while (token != NULL && argc < ARGS - 1) {
          args[argc++] = strdup(token);
          token = strsep(&line_cpy, " ");
        }
        // null terminate args
        args[argc] = NULL;
        int i = 0;

        while (paths[i] != NULL) {
          snprintf(exec_path, sizeof(exec_path), "%s/%s", paths[i], args[0]);
          if (access(exec_path, X_OK) == 0) {
            found = true;
            break;
          }
          i++;
        }

        if (!found) {
          // All paths do not work
          write(STDERR_FILENO, error_message, strlen(error_message));
          exit(1);
        }

        // Execute command
        execv(exec_path, args);
        // execv only returns if failed so everything below is the check
        write(STDERR_FILENO, error_message, strlen(error_message));

        exit(1);

      } else {
        int status;
        wait(&status);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
          write(STDERR_FILENO, error_message, strlen(error_message));
        }
      }
    }
  }

  // Teardown
  free_paths(paths);
  free(line);

  exit(0);
}

void free_paths(char *paths[]) {
  for (int i = 0; i < PATHS; i++) {
    if (paths[i] != NULL) {
      free(paths[i]);
      paths[i] = NULL;
    }
  }
}
