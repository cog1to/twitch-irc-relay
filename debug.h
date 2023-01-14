#include <stdarg.h>
#include <stdio.h>

// Supported log levels.
#define LOG_LEVEL_DEBUG  1
#define LOG_LEVEL_ERROR  2
#define LOG_LEVEL_SILENT 9

extern int LOG_LEVEL;

void LOG(int level, char *fmt, ...);
