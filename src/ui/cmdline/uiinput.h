#ifndef INC_1OOM_UIINPUT_H
#define INC_1OOM_UIINPUT_H

struct input_list_s {
    int value;
    const char *key;
    const char *str;
    const char *display;
};

extern char *ui_input_line(const char *prompt);
extern int ui_input_list(const char *title, const char *prompt, const struct input_list_s *list);

#endif
