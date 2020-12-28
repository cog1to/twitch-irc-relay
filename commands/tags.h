#ifndef TAGS_HEADER
#define TAGS_HEADER

/**
 * Gets value of the specified tag from a tags string.
 *
 * @param input: Tags string.
 * @param tag: Tag to search.
 * @param output: String pointer to hold the tag value.
 * @param size: Max string length of the output.
 **/
int tags_get_tag(char *input, char *tag, char *output, int size);

/**
 * Checks if there's a tag containing given value within the tags string.
 *
 * @param input: Tags string to parse.
 * @param tag: Tag name.
 * @param substring to search within tag value.
 *
 * @returns: 1 if value is found, 0 if not.
 **/
int tags_tag_contains(char *input, char *tag, char *substring);

#endif
