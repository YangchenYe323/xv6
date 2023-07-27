#include "kernel/types.h"

#include "kernel/param.h"
#include "user/user.h"

#define ERR_INPUT_TOO_LONG -234

int read_line(char *buffer, int buf_len, int fd);

int main(int argc, char *argv[]) {
  // Holds augmented argument list
  char *args[MAXARG];
  // Holds input line
  char buffer[512];

  // argv[0] is the xargs executable, skip it
  memmove(args, &argv[1], (argc - 1) * sizeof(char *));
  args[argc - 1] = buffer;

  int bytes_read;
  while ((bytes_read = read_line(buffer, 512, 0)) > 0) {
    if (fork() == 0) {
      exec(args[0], args);
    } else {
      int pid;
      wait(&pid);
    }
  }

  return 0;
}

int read_line(char *buffer, int buf_len, int fd) {
  int code;
  int bytes_read = 0;

  char *buf = buffer;
  while (1) {
    if (bytes_read == buf_len) {
      return ERR_INPUT_TOO_LONG;
    }

    code = read(fd, buf, 1);
    if (code == 0) {
      *buf = '\0';
      return bytes_read;
    }

    if (code < 0) {
      return code;
    }

    if (*buf == '\n') {
      *buf = '\0';
      return bytes_read;
    }

    bytes_read++;
    buf++;
  }
}
