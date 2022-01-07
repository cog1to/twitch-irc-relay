#ifndef UTILS_H_
#define UTILS_H

/**
 * Returns max of a list of int pointers.
 *
 * @parm first: First argument.
 * @param args: Variadic list of rest of the list. Must end with NULL.
 *
 * @return: Max value from the list.
 */
int var_max_int(int *first, ...);

/**
 * Quote-escapes input string and puts it into output string.
 * If resulting string is larger than output, the string is truncated.
 *
 * @param in: Input string.
 * @param out: Output string.
 * @param outsize: Byte-length of the output buffer.
 */
void string_quote_escape(char *in, char *out, int outsize);


#endif
