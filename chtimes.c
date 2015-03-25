/* chtimes.c */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>   /* strerror */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

int chtimes(int delta_seconds, const char *filename)
{
	struct stat statbuf;
	time_t a, m, c;
	struct utimbuf timebuf;

	if (stat(filename, &statbuf) < 0)
		return -1;

	a = statbuf.st_atime;
	m = statbuf.st_mtime;
	c = statbuf.st_ctime;

	timebuf.actime  = a + delta_seconds;
	timebuf.modtime = m + delta_seconds;

	return utime(filename, &timebuf);
}

int main(int argc, const char *argv[])
{
	enum { USAGE=1, BAD_VALUE, BAD_CHTIMES };
	int n, delta_seconds;

	if (argc < 3) {
		printf("Usage: chtimes delta-seconds file...\n"
		       "Purpose: increases access/modify time for given file(s)\n");
		return USAGE;
	}

	delta_seconds = atoi(argv[1]);
	if (!delta_seconds) {
		printf("Delta-seconds value should be non-zero numeric!\n");
		return BAD_VALUE;
	}

	for (n = 2; n < argc; n++) {
		if (chtimes(delta_seconds, argv[n]) < 0) {
			printf("Cannot change times for \"%s\": %s (code %d)\n",
				argv[n], strerror(errno), errno);
			return BAD_CHTIMES;
		}
	}

	return 0;
}

/* EOF */
