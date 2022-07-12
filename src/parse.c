#include "parse.h"

int fm_parse_string(char **input, char *str) {
    int o = 0;
    
    for (char c = str[0]; c != 0; c = str[o]) {
        char ci = (*input)[o++];

        if (c != ci) {
            return 0;
        }
    }

    (*input) += o;

    return 1;
}

int fm_parse_int(char **input, int *out) {
    char *ep;
    
    long l = strtol(*input, &ep, 10);
    if (ep == *input) return 0;
    
    *input = ep;
    *out = (int) l;

    return 1;
}

int fm_parse_float(char **input, float *out) {
    char *ep;
    
    float f = strtof(*input, &ep);
    if (ep == *input) return 0;
    
    *input = ep;
    *out = f;

    return 1;
}

int fm_parse_spaces(char **input) {
    char ch;
    int n = 0;
    
    while (1) {
        ch = (*input)[0];
        
        if (ch == '#') {
            for (ch = (*input)[0]; (*input)[0] != '\0'; (*input)++) n++;
            return n;
        }

        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') return n;

        (*input)++;
        n++;
    }

    return n;
}

int fm_parse_eol(char **input) {
    fm_parse_spaces(input);
    return (*input)[0] == '\0';
}
