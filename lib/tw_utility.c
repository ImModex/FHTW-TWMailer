#include "tw_utility.h"  

int match(char* string, const char *regex_str) {
    regex_t regex;

    if(regcomp(&regex, regex_str, 0) != 0) return -1;
    int comp = regexec(&regex, string, 0, NULL, 0);

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