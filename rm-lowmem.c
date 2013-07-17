/*
 * This file is part of lowmem-tools.
 *
 * lowmem-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * lowmem-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lowmem-tools.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <linux/version.h>

#define ERROR_SIZE 4096
#define GETDENTS_BUF_SIZE 32768

struct linux_dirent {
  unsigned long  d_ino;
  unsigned long  d_off;
  unsigned short d_reclen;
  char           d_name[];
};

static int verbose = 0;
static int filesonly = 0;
static char error_message[ERROR_SIZE];

static int unlink_recursive(const char *dirname) {
  int fd, nread, bpos, rc=0;
  struct linux_dirent *d;
  char buf[GETDENTS_BUF_SIZE];
  char path[PATH_MAX];

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
      if(strcmp(".", d->d_name) == 0 || strcmp("..", d->d_name) == 0)
        continue;
      snprintf(path, sizeof(path), "%s/%s", dirname, d->d_name);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,4)
      int is_dir = *(((char *) d) + d->d_reclen - 1) == DT_DIR;
#else
      struct stat st;
      if(stat(path, &st) == -1) {
        perror(path);
        rc = 1;
        continue;
      }
      int is_dir = S_ISDIR(st.st_mode);
#endif
      if(is_dir) {
        if(!filesonly)
          unlink_recursive(path);
      } else {
        if(verbose)
          printf("removing %s\n", path);
        if(unlinkat(fd, d->d_name, 0) == -1) {
          perror(path);
          rc = 1;
        }
      }
    }
  }

out:
  if(close(fd) == -1) {
    perror(dirname);
    rc = 1;
  }
  if(!filesonly && rc == 0) {
    if(verbose)
      printf("removing %s/\n", dirname);
    if(rmdir(dirname) == -1) {
      perror(dirname);
      rc = 1;
    }
  }

  return rc;
}

int main(int argc, char *argv[]) {
  int i, opt, fail=0;
  const char *dirname;

  while((opt = getopt(argc, argv, "vf")) != -1) {
    switch(opt) {
    case 'v':
      verbose = 1;
      break;
    case 'f':
      filesonly = 1;
      break;
    default:
usage:
      fprintf(stderr, "\
Usage: %s [-v] [-f] directory ...\n\
Options:\n\
  -v  Be verbose\n\
  -f  Only remove files, do not recurse into subdirectories\n",
        argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if(optind >= argc) {
    fprintf(stderr, "%s: missing operand\n", argv[0]);
    goto usage;
  }

  for(i = optind; i < argc; ++i) {
    dirname = argv[i];
    fail |= unlink_recursive(dirname);
  }

  exit(fail ? EXIT_FAILURE : EXIT_SUCCESS);
}
