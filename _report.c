/*
 *  _report.c
 *
 *  Low-level routine for reporting messages in console mode.
 *  Used internally by middle-level report_xxx() routines.
 */

#include <stdio.h>
#include <stdarg.h>

#include "report.h"
#include "_report.h"

static const char* const level2str(int level)
{
	switch(level) {
		case FATAL:   return "FATAL";
		case ERROR:   return "ERROR";
		case WARNING: return "Warning";
		case INFO:    return "Information";
		case NOTIFY:  return "Notification";
	}
	if (level >= DEBUG) {
		static char buf[32];
		snprintf(buf, 32, "Debug(%d)", level);
		return buf;
	}
	return "(?unknown?)";
}

int _report(int level, const char *where, const char *what, va_list ap)
{
	if (level <= report_level) {
		char buf[1024];
		int len = vsnprintf(buf, sizeof(buf)-1, what, ap);
		buf[len] = '\0';
		fprintf(stderr, "%s: %s: %s\n", level2str(level), where, buf);
	}
	return level;
}

/* EOF */
