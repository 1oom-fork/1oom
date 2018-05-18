#include "config.h"

#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "uiinput.h"
#include "lib.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

static bool match_input(const char *in, const char *key, const char *str)
{
    int len = strlen(in);
    if (len == 0) {
        return false;
    }
    if (key && (in[1] == 0) && (tolower(in[0]) == tolower(key[0]))) {
        return true;
    }
    if (str) {
        int i;
        for (i = 0; (i < len) && str[i]; ++i) {
            if (str[i] != tolower(in[i])) {
                return false;
            }
        }
        if (str[i] == 0) {
            return true;
        }
    }
    return false;
}

/* -------------------------------------------------------------------------- */

#ifdef HAVE_READLINE
char *ui_input_line(const char *prompt)
{
    static char *line = NULL;
    lib_free(line);
    line = readline(prompt);
    if ((line != 0) && (*line != 0)) {
        add_history(line);
    }
    return line;
}
#else
char *ui_input_line(const char *prompt)
{
    static char line[1024];
    line[sizeof(line) - 1] = 0;
    fputs(prompt, stdout);
    fflush(stdout);
    return fgets(line, sizeof(line) - 1, stdin);
}
#endif

int ui_input_list(const char *title, const char *prompt, const struct input_list_s *list)
{
    const struct input_list_s *l;
    char *in;
    int num_shortcuts = 0;
    char **shortcuts = NULL;

    l = list;
    while (l->display) {
        ++num_shortcuts;
        ++l;
    }
    shortcuts = lib_malloc(num_shortcuts * sizeof(char *));
    l = list;
    for (int i = 0; i < num_shortcuts; ++i, ++l) {
        const char *s = l->display;
        char *p;
        char c;
        if (*s == '(') { ++s; }
        p = shortcuts[i] = lib_stralloc(s);
        while ((c = *p) != '\0') {
            *p++ = tolower(c);
        }
    }
    for (int i = 0; i < num_shortcuts; ++i) {
        int len;
        bool is_unique;
        is_unique = false;
        len = 0;
        while (!is_unique && shortcuts[i][++len]) {
            is_unique = true;
            for (int j = 0; j < num_shortcuts; ++j) {
                if ((i != j) && (strncmp(shortcuts[i], shortcuts[j], len) == 0)) {
                    is_unique = false;
                    break;
                }
            }
        }
        shortcuts[i][len] = '\0';
    }

    while (1) {
        int i;
        putchar('\n');
        fputs(title, stdout);
        putchar('\n');
        l = list;
        while ((l->str) || (l->key)) {
            if (l->display) {
                fprintf(stdout, "  %s) %s\n", l->key, l->display);
            }
            ++l;
        }
        in = ui_input_line(prompt);
        l = list;
        i = 0;
        while ((l->str) || (l->key)) {
            if (match_input(in, l->key, l->str ? l->str : shortcuts[i])) {
                for (int j = 0; j < num_shortcuts; ++j) {
                    lib_free(shortcuts[j]);
                }
                lib_free(shortcuts);
                return l->value;
            }
            ++l;
            ++i;
        }
        fputs("???\n", stdout);
    }
}
