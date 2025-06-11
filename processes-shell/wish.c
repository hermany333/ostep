#include <fcntl.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PATHS 16
#define CMDS 16
#define ARGS 16
#define MAX_PARALLEL 16

enum BUILTIN { EXIT, SUCCESS, FAILURE };

typedef struct {
  int is_builtin;
  enum BUILTIN code;
  char *exec_line;
} ProcCode;

void null_paths(char *[]);
void free_paths(char *[]);
ProcCode proc_cmd(char *, char *[]);
void pp_cmd(char **);
void pp_line(char *[], char **);
void free_cmds(char *[]);

// Globals
char error_message[30] = "An error has occurred\n";

int main(int argc, char *argv[]) {
  char *line = NULL;
  size_t len = 0;
  char *paths[PATHS] = {NULL};
  char *cmds[CMDS] = {NULL};
  FILE *input = stdin;
  paths[0] = strdup("/bin");

  if (argc > 1) {

    if (argc >= 3) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }

    input = fopen(argv[1], "r");

    if (input == NULL) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
  }

  while (true) {
    // Get user input
    size_t nread;

    if (input == stdin) {
      // CLI display
      fprintf(stdout, "wish> ");
      fflush(stdout);
    }

    nread = getline(&line, &len, input);

    if (nread == -1) {
      exit(0);
    }

    line[strcspn(line, "\n")] = '\0';

    // [char *cmd1, char *cmd2, char *cmd3]
    pp_line(cmds, &line);

    int i = 0;
    pid_t pids[MAX_PARALLEL];
    int p_count = 0;

    while (cmds[i] != NULL) {
      pp_cmd(&cmds[i]);
      ProcCode pc = proc_cmd(cmds[i], paths);

      if (pc.is_builtin) {
        if (pc.code == EXIT) {
          // Teardown
          free_paths(paths);
          free_cmds(cmds);
          free(line);
          if (input != stdin)
            fclose(input);
          exit(0);
        } else {
          break;
        }
      } else {
        // binaries through exec
        int rc = fork();

        if (rc < 0) {
          write(STDERR_FILENO, error_message, strlen(error_message));
          continue;
        } else if (rc == 0) {
          char *args[ARGS];
          int argc = 0;
          char exec_path[512];
          char *line_cpy = pc.exec_line;
          char *token = strsep(&line_cpy, " ");
          bool found = false;
          char *file_arg = NULL;

          while (token != NULL && argc < ARGS - 1) {
            // Check if token is redirection op '>'
            if (strcmp(token, ">") == 0) {
              file_arg = strsep(&line_cpy, " ");

              if (file_arg == NULL || strlen(file_arg) == 0) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
              }

              char *extra = strsep(&line_cpy, " ");

              if (extra != NULL && strlen(extra) > 0) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
              }

            } else {
              args[argc++] = strdup(token);
            }
            token = strsep(&line_cpy, " ");
          }
          free(line_cpy);

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

          // Create new file if > is user
          if (file_arg != NULL) {
            int file_fd = open(file_arg, O_WRONLY | O_CREAT | O_TRUNC, 0666);

            if (file_fd < 0) {
              write(STDERR_FILENO, error_message, strlen(error_message));
              exit(1);
            }

            dup2(file_fd, STDOUT_FILENO);

            close(file_fd);
          }

          // Execute command
          execv(exec_path, args);

          // execv only returns if failed so everything below is the check
          exit(1);
        } else {
          pids[p_count++] = rc;
        }
      }
      i++;
    }

    for (int i = 0; i < p_count; i++) {
      waitpid(pids[i], NULL, 0);
    }
  }

  // Teardown
  free_paths(paths);
  free_cmds(cmds);
  free(line);

  if (input != stdin)
    fclose(input);

  exit(0);
}

void free_cmds(char *cmds[]) {
  for (int i = 0; i < CMDS; i++) {
    if (cmds[i] != NULL) {
      free(cmds[i]);
      cmds[i] = NULL;
    }
  }
}

void free_paths(char *paths[]) {
  for (int i = 0; i < PATHS; i++) {
    if (paths[i] != NULL) {
      free(paths[i]);
      paths[i] = NULL;
    }
  }
}

ProcCode proc_cmd(char *line, char *paths[]) {
  ProcCode pc = {0};
  char *line_cpy = strdup(line);
  char *cmd = strsep(&line_cpy, " ");

  if (cmd == NULL || cmd[0] == '\0') {
    pc.is_builtin = 1;
    pc.code = SUCCESS;
    return pc;
  } else if (strcmp(cmd, "exit") == 0) {
    pc.is_builtin = 1;

    char *extra = strsep(&line_cpy, " ");

    if (extra != NULL && strlen(extra) > 0) {
      pc.code = FAILURE;
      write(STDERR_FILENO, error_message, strlen(error_message));
      return pc;
    }

    pc.code = EXIT;

    return pc;
  } else if (strcmp(cmd, "cd") == 0) {
    pc.is_builtin = 1;

    char *cd_arg = strsep(&line_cpy, " ");
    if (cd_arg == NULL) {
      pc.code = FAILURE;
      write(STDERR_FILENO, error_message, strlen(error_message));
      return pc;
    }

    int res = chdir(cd_arg);

    if (res == -1) {
      pc.code = FAILURE;
      write(STDERR_FILENO, error_message, strlen(error_message));
      return pc;
    }
    pc.code = SUCCESS;

    return pc;
  } else if (strcmp(cmd, "path") == 0) {
    // null out path array if this is not the first time in the same session
    pc.is_builtin = 1;
    pc.code = SUCCESS;
    free_paths(paths);

    char *path_arg;
    int i = 0;
    while ((path_arg = strsep(&line_cpy, " ")) != NULL) {
      paths[i++] = strdup(path_arg);
    }
    return pc;
  }

  pc.is_builtin = 0;
  pc.code = SUCCESS;
  pc.exec_line = strdup(line);
  return pc;
}

void pp_cmd(char **cmd_ptr) {
  char *cmd = *cmd_ptr;
  int i = 0;
  int j = 0;
  char buffer[1024] = {0};

  // Trim trailing spaces at front
  while (*(cmd + i) == ' ')
    i++;

  // Trim trailing spaces at the end
  int len = strlen(cmd);

  while (len > 0 && (cmd[len - 1] == ' ' || cmd[len - 1] == '\t')) {
    cmd[len - 1] = '\0';
    len--;
  }

  char *start_cmd = cmd + i;

  for (int k = 0; start_cmd[k] != '\0'; k++) {
    if (start_cmd[k] == '>') {
      if (k > 0 && start_cmd[k - 1] != ' ')
        buffer[j++] = ' ';

      buffer[j++] = '>';

      if (start_cmd[k + 1] != ' ' && start_cmd[k + 1] != '\0')
        buffer[j++] = ' ';

    } else {
      buffer[j++] = start_cmd[k];
    }
  }

  buffer[j] = '\0';

  free(*cmd_ptr);
  *cmd_ptr = strdup(buffer);
}

void pp_line(char *cmds[], char **line_ptr) {
  char *line = *line_ptr;

  // Trim trailing spaces at front
  while (*line == ' ')
    line++;

  // Trim trailing spaces at the end
  int len = strlen(line);

  while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t')) {
    line[len - 1] = '\0';
    len--;
  }

  char *line_cpy = strdup(line);
  int i = 0;
  char *cmd = NULL;

  while ((cmd = strsep(&line_cpy, "&")) != NULL) {
    if (strlen(cmd) == 0)
      continue;

    cmds[i++] = strdup(cmd);
  }

  // Null termiante array
  cmds[i] = NULL;

  free(*line_ptr);
  *line_ptr = NULL;
}
