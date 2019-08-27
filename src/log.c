#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "log.h"
#include "hw.h"

/* ------------------------------------------------------------------------- */

#define MAX_MSG_LEN (1024 * 2)

static char msgbuf[MAX_MSG_LEN] = "";
bool log_direct_enabled = true;

/* ------------------------------------------------------------------------- */

void log_message_direct(const char *msg)
{
    if (log_direct_enabled) {
        fputs(msg, stdout);
    }
}

void log_warning_direct(const char *msg)
{
    if (log_direct_enabled) {
        fputs(msg, stderr);
    }
}

void log_error_direct(const char *msg)
{
    if (log_direct_enabled) {
        fputs(msg, stderr);
    }
}

void log_message(const char* format, ...)
{
    va_list ap;

    va_start(ap, format);
    vsnprintf(msgbuf, MAX_MSG_LEN, format, ap);
    va_end(ap);

    log_message_direct(msgbuf);
}

void log_warning(const char *format, ...)
{
    va_list ap;
    int len;

    len = snprintf(msgbuf, MAX_MSG_LEN, "warning: ");

    va_start(ap, format);
    vsnprintf(&msgbuf[len], MAX_MSG_LEN - len, format, ap);
    va_end(ap);

    log_warning_direct(msgbuf);
}

void log_error(const char *format, ...)
{
    va_list ap;
    int len;

    len = snprintf(msgbuf, MAX_MSG_LEN, "error: ");

    va_start(ap, format);
    vsnprintf(&msgbuf[len], MAX_MSG_LEN - len, format, ap);
    va_end(ap);

    log_error_direct(msgbuf);
}

void log_fatal_and_die(const char *format, ...)
{
    va_list ap;
    int len;

    len = snprintf(msgbuf, MAX_MSG_LEN, "FATAL: ");

    va_start(ap, format);
    vsnprintf(&msgbuf[len], MAX_MSG_LEN - len, format, ap);
    va_end(ap);

    log_error_direct(msgbuf);
    exit(EXIT_FAILURE);
}
