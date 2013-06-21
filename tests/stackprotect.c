/*
 * verify that dss has been compiled with some form of -fstack-protector
 * -> ensure to configure dss with --enable-stack-protect
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>

static void buffer_overflow()
{
  char a = 'f';
  char s[5];
  int i = 7;
  strcpy(s, "abcdefghi"); /* this will ABRT */
  //strcpy(s, "abcdefghiijklmnopqrst"); /* this will SEGV, no stacktrae */
  printf("%s %d %c\n", s, i, a);
}

int main()
{
  struct rlimit rlim;
  rlim.rlim_cur = RLIM_INFINITY;
  rlim.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_CORE, &rlim) != 0) {
    fprintf(stderr, "Could not configure coredump size limit: %s\n",
            strerror(errno));
    exit(-1);
  }

  printf("Check output for the following items: \n\n"
         "\t1. *** stack smashing detected ***\n"
         "\t2. Aborted (core dumped)\n\n"
         "Starting test\n\n");
  buffer_overflow();
  return 0;
}

