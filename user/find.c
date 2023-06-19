#include "kernel/types.h"

#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

/**
 * Find the component in the path behind last '/'
 */
char *fmtname(char *path) {
  char *p;
  for (p = path + strlen(path); p >= path && *p != '/'; p--) {
  }
  p++;
  return p;
}

int find(char *dir, char *target) {
  // Current file descriptor
  int fd;
  // Current file status
  struct stat st;
  // Directory entry
  struct dirent de;

  char buf[512];
  char *p;

  if ((fd = open(dir, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", dir);
    return -1;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", dir);
    close(fd);
    return -1;
  }

  switch (st.type) {
  case T_DEVICE:
  case T_FILE:
    if (strcmp(fmtname(dir), target) == 0) {
      printf("%s\n", dir);
    }
    /* code */
    break;
  case T_DIR:
  default:
    // Walk the directory
    // Current Path + / + Directory Entry + '\0' = Next Path
    if (strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf) {
      fprintf(2, "find: path too long\n");
      return -1;
    }
    strcpy(buf, dir);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0 || strcmp(de.name, ".") == 0 ||
          strcmp(de.name, "..") == 0) {
        continue;
      }
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, target);
    }
    break;
  }

  close(fd);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(2, "usage: find [root_directory] <file_name>\n");
    return -1;
  }

  if (argc == 2) {
    return find(".", argv[1]);
  }

  return find(argv[1], argv[2]);
}