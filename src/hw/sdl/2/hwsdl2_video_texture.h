#ifndef INC_1OOM_HWSDL2_TEXTURE_H
#define INC_1OOM_HWSDL2_TEXTURE_H

struct SDL_Renderer;
struct hwsdl_video_buffer_s;

/* (re)allocates a texture if needed and uploads supplied buffer into it */
void hwsdl_texture_update(struct SDL_Renderer *, const struct hwsdl_video_buffer_s *buf);

/* put texture on screen */
void hwsdl_texture_output(struct SDL_Renderer *);

/* delete if exists */
void hwsdl_texture_delete(void);

#endif /* INC_1OOM_HWSDL2_TEXTURE_H */

