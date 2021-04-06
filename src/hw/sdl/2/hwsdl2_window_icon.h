#ifndef INC_1OOM_HWSDL2_WINDOW_ICON_H
#define INC_1OOM_HWSDL2_WINDOW_ICON_H

struct SDL_Window;

void hwsdl_set_icon(struct SDL_Window *window);
void hwsdl_delete_icon(void);

#endif

