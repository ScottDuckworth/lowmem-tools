#define _GNU_SOURCE
#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#define ERROR_SIZE 4096
#define GETDENTS_BUF_SIZE 32768

struct linux_dirent {
  unsigned long  d_ino;
  unsigned long  d_off;
  unsigned short d_reclen;
  char           d_name[];
};

static char error_message[ERROR_SIZE];

static int ls_lowmem(const char *dirname) {
  int fd, nread, bpos, rc=0;
  char buf[GETDENTS_BUF_SIZE];
  struct linux_dirent *d;

  fd = open(dirname, O_RDONLY | O_DIRECTORY);
  if(fd == -1) {
    perror(dirname);
    return 1;
  }

  for(;;) {
    nread = syscall(SYS_getdents, fd, buf, sizeof(buf));
    if(nread == -1) {
      snprintf(error_message, sizeof(error_message), "getdents failed on %s", dirname);
      rc = 1;
      goto out;
    }
    if(nread == 0)
      break;

    for(bpos = 0; bpos < nread; bpos += d->d_reclen) {
      d = (struct linux_dirent *)(buf + bpos);
      printf("%s\n", d->d_name);
    }
  }

out:
  if(close(fd) == -1) {
    perror(dirname);
    rc = 1;
  }

  return rc;
}

int main(int argc, char *argv[]) {
  int i, fail=0;
  const char *dirname;

  if(argc == 1) {
    ls_lowmem(".");
  } else {
    for(i = 1; i < argc; ++i) {
      dirname = argv[i];
      if(argc > 2) {
        printf("%s:\n", dirname);
        ls_lowmem(dirname);
        printf("\n");
      } else {
        fail |= ls_lowmem(dirname);
      }
    }
  }

  exit(fail ? EXIT_FAILURE : EXIT_SUCCESS);
}
