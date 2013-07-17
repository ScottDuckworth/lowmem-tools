#define _GNU_SOURCE
#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define BUF_SIZE 32768

struct linux_dirent {
  unsigned long  d_ino;
  unsigned long  d_off;
  unsigned short d_reclen;
  char           d_name[];
};

int main(int argc, char *argv[]) {
  int fd, nread;
  char buf[BUF_SIZE];
  struct linux_dirent *d;
  int bpos, i;
  const char *dirname;

  for(i = 1; i < argc; ++i) {
    dirname = argv[i];
    fd = open(dirname, O_RDONLY | O_DIRECTORY);
    if(fd == -1)
      handle_error(dirname);

    for(;;) {
      nread = syscall(SYS_getdents, fd, buf, BUF_SIZE);
      if(nread == -1)
        handle_error(dirname);
      if(nread == 0)
        break;

      for(bpos = 0; bpos < nread; bpos += d->d_reclen) {
        d = (struct linux_dirent *)(buf + bpos);
        printf("%s\n", d->d_name);
      }
    }

    if(close(fd) == -1)
      handle_error(dirname);
  }

  exit(EXIT_SUCCESS);
}
