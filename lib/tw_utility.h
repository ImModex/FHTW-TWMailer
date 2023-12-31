#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT_LENGTH 1024

int match(char* string, const char *regex_str);
char* readline(char* prompt);
char* reformat_string(char* str);
char* str_to_lower(char* str);
char** split_data(char* data);
void free_data(char*** split);