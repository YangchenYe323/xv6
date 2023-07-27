#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define N 5
char buf[N];

void
pong(int *parent_to_child, int *child_to_parent) {
  if (read(parent_to_child[0], buf, N) < 0) {
    printf("read failed\n");
  }
  printf("%d: received %s\n", getpid(), buf);
  if (write(child_to_parent[1], "pong", 4) != 4) {
    printf("write failed\n");
  }
}

void
ping(int *parent_to_child, int *child_to_parent) {
  
  if (write(parent_to_child[1], "ping", 4) != 4) {
    printf("write failed\n");
  }
  if (read(child_to_parent[0], buf, N) < 0) {
    printf("read failed\n");
  }
  printf("%d: received %s\n", getpid(), buf);
}

int
main(int argc, char *argv[])
{
  int parent_to_child[2];
  int child_to_parent[2];

  int pid;

  if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
    printf("pipe failed\n");
  }
  if ((pid = fork()) < 0) {
    printf("fork failed\n");
  }
  if (pid == 0) {
    pong(parent_to_child, child_to_parent);
  } else {
    ping(parent_to_child, child_to_parent);
  }
  
  exit(0);
}
=======
#include "user/user.h"

int main(int argc, char *argv[]) {
  // Parent send - Child receive
  int pipe1[2];
  // Child send - parent receive
  int pipe2[2];

  if (pipe(pipe1) < 0) {
    fprintf(2, "Error creating pipe\n");
    exit(1);
  }

  if (pipe(pipe2) < 0) {
    fprintf(2, "Error creating pipe\n");
    exit(1);
  }

  int f = fork();
  char byte;

  if (f == 0) {
    // This is the child process
    int exit_code = 0;
    int pid = getpid();

    // We use pipe1[0] for read and pipe2[1] for write
    close(pipe1[1]);
    close(pipe2[0]);

    if (read(pipe1[0], &byte, 1) <= 0) {
      fprintf(2, "%d: read error\n", pid);
      exit_code = -1;
      goto child_exit;
    }

    printf("%d: received ping\n", pid);

    if (write(pipe2[1], &byte, 1) != 1) {
      fprintf(2, "%d: write error\n", pid);
      exit_code = -1;
    }
  child_exit:
    close(pipe1[0]);
    close(pipe2[1]);
    exit(exit_code);

  } else if (f > 0) {
    // This is the parent process
    int exit_code = 0;
    int pid = getpid();

    // We use pipe1[1] for write and pipe2[0] for read
    close(pipe1[0]);
    close(pipe2[1]);

    char byte = 0;
    int i;
    if ((i = write(pipe1[1], &byte, 1)) != 1) {
      printf("(%d)\n", i);
      fprintf(2, "%d: write error\n", pid);
      exit_code = -1;
      goto parent_exit;
    }

    if (read(pipe2[0], &byte, 1) <= 0) {
      fprintf(2, "%d: read error\n", pid);
      exit_code = -1;
      goto parent_exit;
    }

    printf("%d: received pong\n", pid);

  parent_exit:
    close(pipe1[1]);
    close(pipe2[0]);
    exit(exit_code);
  } else {
    fprintf(2, "Fork Error\n");
    exit(-1);
  }

  return 0;
}
