#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "utils.h"

int var_max_int(int *first, ...) {
  va_list args;
  int result = *first;
  va_start(args, first);
  int *par = va_arg(args, int*);
  while (par != NULL) {
    if (result < *par) {
      result = *par;
    }
    par = va_arg(args, int*);
  }
  va_end(args);
  return result;
}

void string_quote_escape(char *in, char *out, int outsize) {
  int out_idx = 0;
  for (int idx = 0; idx < strlen(in) && out_idx < outsize - 1; idx++) {
    if (in[idx] == '"') {
      out[out_idx++] = '\\';
    }
    if (in[idx] == '\\') {
      out[out_idx++] = '\\';
    }

    if (out_idx < (outsize - 1)) {
      out[out_idx] = in[idx];
      out_idx += 1;
    }
  }

  out[out_idx] = '\0';
}

