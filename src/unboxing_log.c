#include <stdarg.h>
#include <stdio.h>

enum BoxingLogLevel {
  BoxingLogLevelInfo = 0,
  BoxingLogLevelWarning = 1,
  BoxingLogLevelError = 2,
  BoxingLogLevelFatal = 3,
  BoxingLogLevelAlways = 4,
};

static const char *coloredLogLevel(const enum BoxingLogLevel level) {
  switch (level) {
  case BoxingLogLevelInfo:
    return "\x1b[36mINFO\x1b[0m   ";
  case BoxingLogLevelWarning:
    return "\x1b[33mWARNING\x1b[0m";
  case BoxingLogLevelError:
    return "\x1b[31mERROR\x1b[0m  ";
  case BoxingLogLevelFatal:
    return "\x1b[31mFATAL\x1b[0m  ";
  case BoxingLogLevelAlways:
    return "\x1b[35mDEBUG\x1b[0m  ";
  }
}

void boxing_log(const enum BoxingLogLevel level,
                const char *const restrict str) {
  printf("%s %s\n", coloredLogLevel(level), str);
}

void boxing_log_args(const enum BoxingLogLevel level,
                     const char *const restrict fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buf[4096];
  const int len = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  printf("%s %.*s\n", coloredLogLevel(level), len, buf);
}
