#ifndef UNBOXING_LOG_C
#define UNBOXING_LOG_C

#include <stdarg.h>
#include <stdio.h>

enum BoxingLogLevel {
  BoxingLogLevelInfo = 0,
  BoxingLogLevelWarning = 1,
  BoxingLogLevelError = 2,
  BoxingLogLevelFatal = 3,
  BoxingLogLevelAlways = 4,
};

static const char *boxing_log_level_str[] = {
    "\x1b[36mINFO\x1b[0m   ", "\x1b[33mWARNING\x1b[0m",
    "\x1b[31mERROR\x1b[0m  ", "\x1b[31mFATAL\x1b[0m  ",
    "\x1b[35mDEBUG\x1b[0m  ",
};

void boxing_log(const enum BoxingLogLevel level,
                const char *const restrict str) {
  fprintf(stderr, "%s %s\n", boxing_log_level_str[level], str);
}

void boxing_log_args(const enum BoxingLogLevel level,
                     const char *const restrict fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buf[4096];
  const int len = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  fprintf(stderr, "%s %.*s\n", boxing_log_level_str[level], len, buf);
}

#endif
