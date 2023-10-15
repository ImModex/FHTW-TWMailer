#include "tw_utility.h"  

int match(char* string, const char *regex_str) {
    regex_t regex;

    if(regcomp(&regex, regex_str, REG_EXTENDED | REG_NEWLINE) != 0) return -1;
    int comp = regexec(&regex, string, 0, NULL, 0);

    printf("Matching str: |%s|\n", regex_str);
    printf("Regex str: |%s|\n", string);
    printf("Compr: %d\n", comp);

    regfree(&regex);
    return comp == 0 ? 1 : 0;
}

char* reformat_string(char* str) {
    int index = 0;
    printf("%s\n", str);
    while(str[index] != '\0') {
        char idk[2];
        idk[0] = str[index];
        idk[1] = '\0';
        if(!match(idk, "[a-zA-Z0-9]")) str[index] = '_';
        ++index;
        printf("%c -> %d\n", str[index], index);
    }

    return str;
}

char* str_to_lower(char* str) {
    for(int i = 0; str[i] != '\0'; i++) {
        str[i] = tolower(str[i]);
    }
    return str;
}

char** split_data(char* data) {
    char** split = (char**) malloc(sizeof(char*));

    if(split == NULL) {
        perror("Memory Error (split_data)");
        exit(1);
    }

    int index = 0;
    char* prevPtr = data;
    char* ptr = data;
    while(*ptr != '\0') {
        while(*(ptr++) != '\n');

        split[index] = calloc((ptr - prevPtr), sizeof(char));
        strncpy(split[index], prevPtr, (ptr - prevPtr) - sizeof(char));

        prevPtr = ptr;
        ++index;

        split = realloc(split, (index + 1) * sizeof(char*));
        split[index] = NULL;
    }

    return split;
}

void free_data(char*** split) {
    char** _split = *split;
    int index = 0;

    while(_split[index] != NULL) {
        free(_split[index++]);
    }

    free(*split);
}