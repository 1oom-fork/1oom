#ifndef INC_1OOM_OPTIONS_H
#define INC_1OOM_OPTIONS_H

#ifdef FEATURE_MODEBUG
extern int opt_modebug;
#endif
extern int opt_audio_enabled;
extern int opt_music_enabled;
extern int opt_sfx_enabled;
extern int opt_music_volume;
extern int opt_sfx_volume;
extern int opt_audiorate;
extern int opt_audioslice_ms;
extern int opt_xmid_banks;
#ifdef HAVE_SAMPLERATE
extern int opt_use_libsamplerate;
extern int opt_libsamplerate_scale;
extern int opt_libsamplerate_mode;
#endif

struct cmdline_options_s {
    const char *str;
    int num_param;
    int (*handle)(char **argv, void *var);
    void *var;
    const char *str_param;
    const char *str_help;
};

extern int options_enable_var(char **argv, void *var);
extern int options_disable_var(char **argv, void *var);
extern int options_set_int_var(char **argv, void *var);
extern int options_set_str_var(char **argv, void *var);

extern int options_parse_early(int argc, char **argv);
extern int options_parse(int argc, char **argv);
extern void options_show_usage(void);

#endif
