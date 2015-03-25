/*
 *  ls4sweep.c
 *
 *  Written by evseev@altlinux.ru
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       /* write() */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>    /* NAME_MAX */
#include <sys/time.h>     /* timezone */
#include <ctype.h>
#include <time.h>
#include <errno.h>

#if 0
#include <fnmatch.h>
#include <dirent.h>
#endif

#include "report.h"

#define PROGRAM_NAME    "GNU ls4sweep"
#define PROGRAM_VERSION "0.3.1"
#define PROGRAM_AUTHOR  "ilya.evseev@gmail.com"
#define PROGRAM_DATE    "Feb 2005, May 2013"


/*========  Common part  ============*/

typedef struct {
	int days_per_period;
	int periods_count;
} Schedule_token;

typedef enum { USE_MODIFY_TIME, USE_CHANGING_TIME, USE_ACCESS_TIME }
	Timetype;

enum { False, True };


static int tzoffset = ((-1) >> 1);

static int time2days(time_t t) { return (int)( t / (24*60*60) ); }
static int get_daynow(void)    { return time2days(time(NULL)); }

static int init_tzoffset(void)
{
	struct timeval tv;
	struct timezone tz;
	int retval = gettimeofday(&tv, &tz);
/*	return report_fatal(fn, "gettimeofday: %s (%d)", strerror(errno), errno); */
	return tzoffset = (60 * tz.tz_minuteswest);
}

static const char* timetype2str(Timetype t) {
	static char s[32];  /* it is not multi-threadable! */
	switch(t) {
		case USE_ACCESS_TIME:   return "USE_ACCESS_TIME";
		case USE_MODIFY_TIME:   return "USE_MODIFY_TIME";
		case USE_CHANGING_TIME: return "USE_CHANGING_TIME";
	}
	sprintf(s, "Unknown %d", t);
	return s;
}

/* Forward declaration */
static int mark4remove(const char *fpath);

static time_t getfiletime(Timetype timetype, const char *filepath)
{
	int result;
	struct stat fileinfo;
	const char func[] = "getfiletime";

	if (stat(filepath, &fileinfo) < 0) {
		report_error(func, "stat(\"%s\") = %s(%d)", filepath, strerror(errno), errno);
		return ((time_t)-1);
	}

	switch (timetype) {
		case USE_ACCESS_TIME:   result = fileinfo.st_atime; break;
		case USE_MODIFY_TIME:   result = fileinfo.st_mtime; break;
		case USE_CHANGING_TIME: result = fileinfo.st_ctime; break;
		default:report_fatal(func, "timetype(\"%s\") = %d", filepath, timetype);
			result = fileinfo.st_mtime;
			break;
	}
	result -= tzoffset;
	report_debug(99, func, "done");
	return result;

}

static int find_oldiest_between(Timetype timetype, int minday, int maxday,
	const char *filenames[], int nfiles)
{
	const char fn[] = "find_oldiest_between";
	size_t fname_index;
	const char *oldiest_filename = NULL;
	int oldiest_days, d;

	report_debug(80, fn, "minday = %d, maxday = %d, %d files",
		minday, maxday, nfiles);

	for (fname_index = 0; fname_index < nfiles; fname_index++) {
		const char *filename = filenames[fname_index];

		time_t filetime = getfiletime(timetype, filename);
		if (filetime == ((time_t)-1))
			continue;
		d = time2days(filetime);
		report_debug(90, fn, "check file #%d: \"%s\", age = %d days",
			fname_index, filename, d);

		if (d <= minday || d > maxday)
			continue;
		if (!oldiest_filename || oldiest_days > d) {
			report_debug(80, fn, "set file \"%s\" as oldiest", filename);
			if (oldiest_filename)
				mark4remove(oldiest_filename);
			oldiest_filename = filename;
			oldiest_days = d;
		} else {
			mark4remove(filename);
		}
	}
}

static void skip_delims(const char **pp)
{
	static const char delims[] = " ;,-:";
	if (!pp || !*pp || ! **pp)
		return;
	while (strchr(delims, **pp))
		(*pp)++;
}

/* Resets (*pp) to NULL on error, i.e, when no digits found at top of (*pp) */
static int scan_number(const char **pp, int defval)
{
	int n = 0, pos = 0;
	while(isdigit(**pp)) {
		n = (n * 10) + ((**pp) - '0');
		pos++;
		(*pp)++;
	}
	if (!pos) {
		if (!**pp)
			*pp = NULL;
		return defval;
	}
	return n;
}

static int scan_koeff(const char **pp)
{
	int k = 24 * 60 * 60;   /* seconds in day */
	switch (**pp) {
		case 'd': case 'D' : break;
		case 'h': case 'H' : k = 60 * 60; break;
		case 'm': case 'M' : k = 60; break;
		case 's': case 'S' : k =  1; break;
		default: return k;
	}
	(*pp)++;
	return k;
}

/* Resets (*srctokens_ptr) to NULL on error */
static Schedule_token *getnext_schedule_token(const char **srctokens_ptr, Schedule_token *result)
{
	const char fn[] = "getnext_schedule_token";
	const char **pp = srctokens_ptr;
	int n1, n2, k;

	report_debug(80, fn, "srctokens = \"%s\"", *pp);

	if (!**pp)
		return NULL;

	if (!result || !pp || !*pp) {
		report_fatal(fn, "Empty pointer");
		*pp = NULL;
		return NULL;
	}

	skip_delims(pp);
	if ((n1 = scan_number(pp, -1)) <= 0) {
		if (*pp)
			report_error(fn, "Invalid periods count in \"%s\"",
				*srctokens_ptr ? *srctokens_ptr : "");
		*pp = NULL;
		return NULL;
	}
	skip_delims(pp);
	if ((n2 = scan_number(pp, -1)) < 0) {
		report_error(fn, "Invalid days count in \"%s\"",
			*srctokens_ptr ? *srctokens_ptr : "");
		*pp = NULL;
		return NULL;
	}
	k = 1; //k = scan_koeff(pp);
	skip_delims(pp);
	result->periods_count = n1;
	result->days_per_period = n2 * k;

	report_debug(80, fn, "done, %d periods, %d per period, tail = \"%s\"", n1, n2, *pp);
	return result;
}

static int isvalid_schedule_policy(const char *policy)
{
	const char func[] = "isvalid_schedule_policy";
	Schedule_token t;
	while (getnext_schedule_token(&policy, &t));
	if (!policy) {
		report_debug(90, func, "False");
		return False;
	}
	report_debug(90, func, "True");
	return True;
}

static int found_actual_backup(const char *schedule_policy, Timetype timetype,
	const char *filenames[], int fnames_count)
{
	const char func[] = "found_actual_backup";

	Schedule_token t;
	const char *schedule_pos = schedule_policy;

	int checking_nearest = True;
	int i, d1 = get_daynow(), result = False;

	report_debug(70, func, "schedule_policy = \"%s\", nfiles = %d",
		schedule_policy, fnames_count);

	while ((getnext_schedule_token(&schedule_pos, &t)) != NULL) {
		int ndays = t.days_per_period;
		int nperiods = t.periods_count;
		int p, d;
		for (p = 0; p < nperiods; p++) {
			int d2 = d1 - ndays;
			if (d2 < 0)
				d2 = 0;
			if (find_oldiest_between(timetype, d2, d1, filenames, fnames_count))
				if (checking_nearest)
					result = True;
			checking_nearest = False;
			if ((d1 = d2) < 0)
				break;
		}
	}
	if (d1 > 0) {
		find_oldiest_between(timetype, 0, d1, filenames, fnames_count);
	}

	report_debug(80, func, "done");
	return result;
}


/*========  Console part  ============*/

#ifndef NO_CONSOLE_APP

static const char *argv0;

struct {
	const char *schedule_policy;
	Timetype timetype;
	int print0;           /* Use \0 as line separator, default is \n */
	int quoted_output;
} options;


static int mark4remove(const char *filepath)
{
	if (options.quoted_output)
		putchar('"');
	printf(filepath);
	if (options.quoted_output)
		putchar('"');
	if (options.print0) {
		static const char zero = '\0';
		fflush(stdout);
		write(1, &zero, 1);
	} else {
		putchar('\n');
	}
	return 0;
}

static int print_version(void)
{
	printf(PROGRAM_NAME " " PROGRAM_VERSION "\n");
	return -1;
}

static void print_header(void)
{
	static int already_printed = 0;
	if (already_printed)
		return;
	printf(PROGRAM_NAME " " PROGRAM_VERSION ", written by " PROGRAM_AUTHOR " at " PROGRAM_DATE "\n");
	already_printed = 0;
}

static int short_usage(const char *errmsg, ...)  /* ..called by parse_switches */
{
	va_list ap;
	va_start(ap, errmsg);
	vreport_error("Usage", errmsg, ap);
	va_end(ap);
	fprintf(stderr, "Use %s --help for getting complete usage description\n", argv0);
	return -1;
}

static int full_usage(void)
{
	print_header();
	printf("Usage: %s [options] 'p1 d1 p2 d2...' filename...\n", argv0);
	printf("Options:\n"
		"    -h, --help		Display complete usage\n"
		"    -d, --debug <level>	Enable verbose output (0=min,100=max)\n"
		"    -m, --use-mtime	Check files by last modification time (default)\n"
		"    -c, --use-ctime	Check files by creation time\n"
		"    -f, --use-format <fmt>	Reserved for future use\n"
		"    -0, --print0	Use ASCII ZERO instead of NEWLINE after filenames\n"
		"    -q, --printq	Quote displayed filenames\n");
	printf("Examples:\n"
		"    %s 7:1,3:7,11:30,100:360 *.zip\n", argv0);
	return -1;
}

static int set_timetype(int val)
{
	if (options.timetype)
		return short_usage("time type options given more than once");
	options.timetype = (Timetype)val;
	return 1;
}

/* Returns count of passed args, typically 1(no additional value) or 2(arg.name+value) */
int parse_long_option(const char *name, int argnum, int argc, const char **argv)
{
	if (strcmp(name, "help") == 0)
		return full_usage();
	if (strcmp(name, "use-mtime") == 0)
		return set_timetype(USE_MODIFY_TIME);
	if (strcmp(name, "use-atime") == 0)
		return set_timetype(USE_ACCESS_TIME);
	if (strcmp(name, "use-ctime") == 0)
		return set_timetype(USE_CHANGING_TIME);
	if (strcmp(name, "use-format") == 0)
		return short_usage("Option --use-format is not implemented yet");
	if (strcmp(name, "version") == 0)
		return print_version();
	if (strcmp(name, "print0") == 0) {
		options.print0 = True;
		return 1;
	}
	if (strcmp(name, "printq") == 0) {
		options.quoted_output = True;
		return 1;
	}
	if (strcmp(name, "debug") == 0) {
		if (argnum >= argc-1)
			return short_usage("Missing debug level value");
		report_level = atoi(argv[argnum+1]);
		return 2;
	}
	return short_usage("Unknown option #%d: %s", argnum, name);
}

/* Returns count of passed args, typically 1 */
int parse_short_options(const char *chars, int argnum, int argc, const char **argv)
{
	int retval = 1;
	const char *p;
	for (p = chars; *p; p++) {
		switch(*p) {
			case 'h': return full_usage();
			case 'V': return print_version();
			case '0': { options.print0 = True;        return 1; }
			case 'q': { options.quoted_output = True; return 1; }
			case 'm': set_timetype(USE_MODIFY_TIME);   break;
			case 'c': set_timetype(USE_CHANGING_TIME); break;
			case 'a': set_timetype(USE_ACCESS_TIME);   break;
			case 'd': {
				const char *valptr = p+1;
				if (*valptr) {
					p = " ";  /* terminate after this iteration */
				} else {
					if (argnum >= argc-1)
						return short_usage("Missing debug level value");
					valptr = argv[argnum+1];
					retval = 2;
				}
				report_level = atoi(valptr);
				break;
			}
			case 'f': return short_usage("Option --use-format is not implemented yet");
			default : return short_usage("Unknown option in #%d: %c", argnum, *p);
		}
	}
	return retval;
}

static void dump_args(void)
{
	int i = 0;
	Schedule_token t;
	const char *p = options.schedule_policy;

	printf("Command line options:\n");
	printf("Time type = %s\n", timetype2str(options.timetype));

	printf("Schedule policy:\n");
	printf("%s\n", options.schedule_policy);
	while (getnext_schedule_token(&p, &t)) {
		i++;
		printf("#%d: %d periods, %d days\n", i, t.periods_count, t.days_per_period);
	}
}

int parse_args(int argc, const char *argv[])
{
	const char func[] = "parse_args";
	int i;
	argv0 = argv[0];

	for (i = 1; i < argc; i++) {
		const char *s = argv[i];
		if (s[0] == '-') {
			int retval;
			if (s[1] == '-') {
				retval = parse_long_option(s+2, i, argc, argv);
			} else {
				retval = parse_short_options(s+1, i, argc, argv);
			}
			if (retval < 0)
				return retval;
			i += retval - 1;
		} else if (options.schedule_policy) {
			report_debug(80, func, "found first filename at position %d", i);
			break;  /* args starting #i are dirpaths */
		} else if (isvalid_schedule_policy(s)) {
			options.schedule_policy = s;
			report_debug(80, func, "found policy \"%s\"", s);
		} else {
			return -1;
		}
	}

	report_debug(1, func, "report_level = %d", report_level);
	if (report_level >= 50)
		dump_args();

	if (!options.schedule_policy)
		return short_usage("Missing policy");
	if (i >= argc)
		return short_usage("Missing path to process, use '.' for current directory");

	return i;
}

int count_existing_files(const char *filenames[], int fnames_count)
{
	struct stat fileinfo;
	int n, total_exists = 0;
	for (n = 0; n < fnames_count; n++)
		if (stat(filenames[n], &fileinfo) >= 0)
			total_exists++;
	return total_exists;
}

int main(int argc, const char *argv[])
{
	int first_dirpath_index, fnames_count;
	const char **filenames;

	if ((first_dirpath_index = parse_args(argc, argv)) < 0)
		return 1;
	init_tzoffset();

	filenames = argv + first_dirpath_index;
	fnames_count = argc-first_dirpath_index;

	if (!count_existing_files(filenames, fnames_count)) {
		report_fatal("main", "no files actually exists.");
		return 1;
	}

	if (found_actual_backup(
			options.schedule_policy, 
			options.timetype,
			filenames,
			fnames_count) > 0)
		return 0; // successful termination, actual backup found

	return 1;  // error occured or actual backup not found
}

#endif   /* ifndef NO_CONSOLE_APP */

/* EOF */
