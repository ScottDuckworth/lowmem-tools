/*
 * Copyright 2013 Clemson University
 *
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
#include <sys/stat.h>
#include <sys/syscall.h>
#include "version.h"

#define ERROR_SIZE 4096
#define GETDENTS_BUF_SIZE 32768

struct linux_dirent {
  unsigned long  d_ino;
  unsigned long  d_off;
  unsigned short d_reclen;
  char           d_name[];
};

static char error_message[ERROR_SIZE];

static void usage(FILE *file, const char *arg0) {
  fprintf(file,
    "Usage: %s [options] [directory] ...\n"
    "Options:\n"
    "  -h  Print this message\n"
    "  -V  Print version\n"
    , arg0);
}

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
  int i, opt, fail=0;
  const char *dirname;

  while((opt = getopt(argc, argv, "hV")) != -1) {
    switch(opt) {
    case 'h':
      usage(stdout, argv[0]);
      exit(0);
    case 'V':
      printf("ls-lowmem (lowmem-tools) " VERSION "\n");
      exit(0);
    default:
      usage(stderr, argv[0]);
      exit(1);
    }
  }

  if(optind == argc) {
    ls_lowmem(".");
  } else {
    for(i = optind; i < argc; ++i) {
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
