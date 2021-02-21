#include <stdlib.h>
#include <stdio.h>
#include "debug.h"

void LOG(int level, char *fmt, ...) {
  if (level >= LOG_LEVEL) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }
}
