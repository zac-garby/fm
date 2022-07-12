#ifndef H_FM_PARSE
#define H_FM_PARSE

#include <string.h>
#include <stdlib.h>

// parses a specific string at the start of the input, or fails.
// returns 1 if the string is found, and 0 otherwise.
int fm_parse_string(char **input, char *str);

// parses the string as an integer. returns 0 if no
// digits are found.
int fm_parse_int(char **input, int *out);

// parses the string as a float. returns 0 if no float is found.
int fm_parse_float(char **input, float *out);

// parses whitespace and comments from the beginning of the string.
// returns the amount of whitespace characters skipped.
int fm_parse_spaces(char **input);

// ensures that the parser is at the end of the line (except possibly
// for whitespace.) returns 0 if more non-whitespace exists.
int fm_parse_eol(char **input);

#endif
