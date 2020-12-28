#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "tags.h"

/** Private **/

int str_prefix(char *str, char *substr) {
  if (strstr(str, substr) == str) {
    return 1;
  } else {
    return 0;
  }
}

int max_int(int a, int b) {
  if (a > b) {
    return a;
  } else {
    return b;
  }
}

int min_int(int a, int b) {
  if (a > b) {
    return b;
  } else {
    return a;
  }
}

/** Public **/

/**
 * Gets value of the specified tag from a tags string.
 *
 * @param input: Tags string.
 * @param tag: Tag to search.
 * @param output: String pointer to hold the tag value.
 * @param size: Max string length of the output.
 *
 * @return: Length of the value string, if the tag was found, 0 otherwise.
 **/
int tags_get_tag(char *input, char *tag, char *output, int size) {
  int length = strlen(input), value_length = 0;
  char *token = NULL, *string, *pointer;

  string = malloc(length * sizeof(char) + 1);
  pointer = string;
  memcpy(string, input, length + 1);
  string[length + 1] = '\0';

  while ((token = strsep(&pointer, ";")) != NULL) {
    if (str_prefix(token, tag)) {
      char *value = strchr(token, '=');
      if (value != NULL) {
        value += 1;
        value_length = min_int(strlen(value), size - 1);
        strncpy(output, value, value_length);
        output[value_length + 1] = '\0';
        return value_length;
      }
    }
  }

  free(string);
  return 0;
}

/**
 * Checks if there's a tag containing given value within the tags string.
 *
 * @param input: Tags string to parse.
 * @param tag: Tag name.
 * @param substring to search within tag value.
 *
 * @return: 1 if value is found, 0 if not.
 **/
int tags_tag_contains(char *input, char *tag, char *substring) {
  char tag_value[128];

  int tag_value_length = tags_get_tag(input, tag, tag_value, 128);
  char *substr = strstr(tag_value, substring);

  if (substr != NULL) {
    return 1;
  } else {
    return 0;
  }
}

