#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "../include/parser.h"

static void skip_ws(const char *s, size_t *i) {
    for ( ; s[*i] && (s[*i] == ' ' || s[*i] == '\t' || s[*i] == '\n') ; (*i)++) {}
}

static bool word(const char *s, size_t *i) {
    skip_ws(s, i);
    size_t j = *i;

    if (!s[j]) return false;

    for ( ; s[j] && !isspace((unsigned char)s[j]) && (unsigned char)s[j] != '|' && (unsigned char)s[j] != '&'
    && (unsigned char)s[j] != '>' && (unsigned char)s[j] != '<' && (unsigned char)s[j] != ';' ; j++) {}

    if (j == *i) return false;
    *i = j;
    return true;
}

static bool input_redir(const char *s, size_t *i) {
    size_t save = *i;
    skip_ws(s, i);
    if (s[*i] != '<') { *i = save; return false; }
    (*i)++;
    
    // after i/p redirection there should be a word exist 
    if (!word(s, i)) {
        *i = save; return false;
    }
    return true;
}

static bool output_redir(const char *s, size_t *i) {
    size_t save = *i;
    skip_ws(s, i);
    if (s[*i] != '>') { *i = save; return false; }
    if (s[*i + 1] == '>') (*i) += 2;
    else (*i)++;

    // after o/p redirection or append there should be a word exist 
    if (!word(s, i)) {
        *i = save; return false;
    }
    return true;
}

static bool atomic(const char *s, size_t *i) {
    
    skip_ws(s, i);
    if (!word(s, i)) return false;

    for (;;) {
        size_t save = *i;
        skip_ws(s, i);
        if (input_redir(s, i) || output_redir(s, i) || word(s, i)) {
            continue;
        }
        *i = save;
        break;
    }
    return true;
}

static bool valid_cmd_seg(const char *s, size_t *i) {

    skip_ws(s, i);
    if (!atomic(s, i)) return false;

    for (;;) {
        size_t save = *i;
        skip_ws(s, i);

        if (s[*i] != '|') {
            *i = save;
            break;
        }
        (*i)++; skip_ws(s, i);
        if (!atomic(s, i)) return false;
    }
    return true;
}

bool parse_shell_cmd(const char *s) {
    size_t i = 0;

    if (!valid_cmd_seg(s, &i)) return false;

    for (;;) {
        size_t save = i;
        skip_ws(s, &i);

        if (s[i] == '&' || s[i] == ';') {
            i++; skip_ws(s, &i);

            if (!valid_cmd_seg(s, &i)) {
                i = save;
                break;
            }
        } else {
            i = save;
            break;
        }
    }

    skip_ws(s, &i);
    if (s[i] == '&' || s[i] == ';') i++;

    skip_ws(s, &i);
    return s[i] == '\0';
}
