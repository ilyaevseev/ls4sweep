/* report.c */

#include <stdio.h>
#include <stdarg.h>

#include "report.h"
#include "_report.h"

int report_level = 0;

#define _RETURN_DOT(_Level) \
	do { \
		int retval; \
		va_list ap; \
		va_start(ap, what); \
		retval = _report(_Level, where, what, ap); \
		va_end(ap); \
		return retval; \
	} while(0)

#define _DOT_ARGS const char *where, const char *what, ...
#define _AP_ARGS  const char *where, const char *what, va_list ap
#define _RETURN_AP(x) return _report(x, where, what, ap)

int report_fatal  (_DOT_ARGS) { _RETURN_DOT(FATAL); }
int report_error  (_DOT_ARGS) { _RETURN_DOT(ERROR); }
int report_warning(_DOT_ARGS) { _RETURN_DOT(WARNING); }
int report_info   (_DOT_ARGS) { _RETURN_DOT(INFO); }
int report_notify (_DOT_ARGS) { _RETURN_DOT(NOTIFY); }
int report_debug  (int level, _DOT_ARGS) { _RETURN_DOT(level); }

int vreport_fatal  (_AP_ARGS) { _RETURN_AP(FATAL); }
int vreport_error  (_AP_ARGS) { _RETURN_AP(ERROR); }
int vreport_warning(_AP_ARGS) { _RETURN_AP(WARNING); }
int vreport_info   (_AP_ARGS) { _RETURN_AP(INFO); }
int vreport_notify (_AP_ARGS) { _RETURN_AP(NOTIFY); }
int vreport_debug  (int level, _AP_ARGS) { _RETURN_AP(level); }

int report_errno  (_DOT_ARGS) { _RETURN_DOT(ERROR); }
int vreport_errno  (_AP_ARGS) { _RETURN_AP(ERROR); }

/* EOF */
