#include "kernel/types.h"
#include "user/user.h"

int child_process();

int main(int argc, char *argv[]) {
  int exit_code = 0;
  // The first process initiates the pipeline and feed the pipeline with 2..35
  int pipefds[2];
  pipe(pipefds);

  if (fork() == 0) {
    close(0);
    close(pipefds[1]);
    dup(pipefds[0]);
    close(pipefds[0]);
    return child_process();
  }

  for (int i = 2; i <= 35; i++) {
    if (write(pipefds[1], &i, 4) != 4) {
      fprintf(2, "Write Error\n");
      exit_code = -1;
      break;
    }
  }

  close(pipefds[0]);
  close(pipefds[1]);
  int child_pid;
  while (wait(&child_pid) != -1) {
  }
  return exit_code;
}

int child_process() {
  int exit_code = 0;
  int initialized = 0;
  int pipefds[2];

  int number;
  int prime;

  while (read(0, &number, 4) > 0) {
    if (!initialized) {
      pipe(pipefds);
      if (fork() == 0) {
        // The child process uses pipefds[0] as the standard input.
        close(pipefds[1]);
        close(0);
        dup(pipefds[0]);
        close(pipefds[0]);
        return child_process();
      }
      prime = number;
      printf("prime %d\n", prime);
      initialized = 1;
      continue;
    }

    // Write the number to the child process
    if (number % prime != 0) {
      if (write(pipefds[1], &number, 4) != 4) {
        exit_code = -1;
        fprintf(2, "Write Error\n");
        break;
      }
    }
  }

  if (initialized) {
    close(pipefds[0]);
    close(pipefds[1]);
    int waited_pid;
    // Wait for child processes to exit
    while (wait(&waited_pid) != -1) {
    }
  }
  return exit_code;
}