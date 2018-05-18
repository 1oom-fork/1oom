#ifndef INC_1OOM_HW_H
#define INC_1OOM_HW_H

/* API to hw/ */

#include "cfg.h"
#include "options.h"
#include "types.h"

extern const char *idstr_hw;

extern int hw_early_init(void);
extern int hw_init(void);
extern void hw_shutdown(void);

extern const struct cmdline_options_s hw_cmdline_options[];
extern const struct cmdline_options_s hw_cmdline_options_extra[];

extern const struct cfg_items_s hw_cfg_items[];
extern const struct cfg_items_s hw_cfg_items_extra[];

extern int hw_event_handle(void);
extern void hw_video_position_cursor(int mx, int my);

extern void hw_textinput_start(void);
extern void hw_textinput_stop(void);

extern int hw_icon_set(const uint8_t *data, const uint8_t *pal, int w, int h);

extern int hw_video_init(int w, int h);
extern void hw_video_refresh_palette(void);
/* Draw the current back buffer and return new back buffer. */
extern uint8_t *hw_video_draw_buf(void);
/* Redraw the front buffer. */
extern void hw_video_redraw_front(void);

extern int hw_audio_music_init(int mus_index, const uint8_t *data, uint32_t len);
extern void hw_audio_music_release(int mus_index);
extern void hw_audio_music_play(int mus_index);
extern void hw_audio_music_fadeout(void);
extern void hw_audio_music_stop(void);
extern void hw_audio_music_volume(int volume/*0..128*/);

extern int hw_audio_sfx_init(int sfx_index, const uint8_t *data, uint32_t len);
extern void hw_audio_sfx_release(int sfx_index);
extern void hw_audio_sfx_play(int sfx_index);
extern void hw_audio_sfx_stop(void);
extern void hw_audio_sfx_volume(int volume/*0..128*/);

extern uint32_t hw_get_time_us(void);

#endif
