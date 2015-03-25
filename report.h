/* report.h */

#include <stdarg.h>  /* va_list */

enum { FATAL=-9, ERROR, WARNING, INFO, NOTIFY, DEBUG=0 };

/* Positive values are used for debugging (0 = off, 100 = most detailed),
 * negative are reserved.
 */
extern int report_level;

int  report_fatal  (const char *where, const char *what, ...);
int vreport_fatal  (const char *where, const char *what, va_list ap);
int  report_error  (const char *where, const char *what, ...);
int vreport_error  (const char *where, const char *what, va_list ap);
int  report_errno  (const char *where, const char *what, ...);
int vreport_errno  (const char *where, const char *what, va_list ap);
int  report_warning(const char *where, const char *what, ...);
int vreport_warning(const char *where, const char *what, va_list ap);
int  report_info   (const char *where, const char *what, ...);
int vreport_info   (const char *where, const char *what, va_list ap);
int  report_notify (const char *where, const char *what, ...);
int vreport_notify (const char *where, const char *what, va_list ap);
int  report_debug  (int level, const char *where, const char *what, ...);
int vreport_debug  (int level, const char *where, const char *what, va_list ap);

/* EOF */
