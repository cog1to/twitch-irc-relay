#include <stdlib.h>
#include <stdio.h>
#include "debug.h"

int LOG_LEVEL = LOG_LEVEL_ERROR;

void LOG(int level, char *fmt, ...) {
  if (level >= LOG_LEVEL) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }
}
