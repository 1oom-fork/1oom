#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"
#include "hw.h"
#include "lib.h"
#include "log.h"
#include "main.h"
#include "os.h"
#include "ui.h"

/* -------------------------------------------------------------------------- */
/* global options */

#ifdef FEATURE_MODEBUG
int opt_modebug = 0;
#endif
int opt_audio_enabled = 1;
int opt_music_enabled = 1;
int opt_sfx_enabled = 1;
int opt_music_volume = 64;
int opt_sfx_volume = 100;
int opt_audiorate = 48000;
int opt_audioslice_ms = 50;
int opt_xmid_ticksperq = 55;
int opt_xmid_banks = 0;
#ifdef HAVE_SAMPLERATE
int opt_use_libsamplerate = 1;
int opt_libsamplerate_scale = 65;
int opt_libsamplerate_mode = 1;
#endif

/* -------------------------------------------------------------------------- */

int options_enable_var(char **argv, void *var)
{
    *((int *)var) = 1;
    return 0;
}

int options_disable_var(char **argv, void *var)
{
    *((int *)var) = 0;
    return 0;
}

int options_set_int_var(char **argv, void *var)
{
    *((int *)var) = atoi(argv[1]);
    return 0;
}

int options_set_str_var(char **argv, void *var)
{
    *(const char **)var = argv[1];
    return 0;
}

int options_notimpl(char **argv, void *var)
{
    log_error("option '%s' is not implemented (yet)", argv[0]);
    return -1;
}

int options_nop(char **argv, void *var)
{
    return 0;
}

/* -------------------------------------------------------------------------- */

static int options_set_datadir(char **argv, void *var)
{
    log_message("Setting data directory to '%s'\n", argv[1]);
    os_set_path_data(argv[1]);
    return 0;
}

/* -------------------------------------------------------------------------- */

static int show_usage(char **argv, void *var);

static const struct cmdline_options_s cmdline_options_early[] = {
    { "-?", 0,
      show_usage, NULL,
      NULL, "Show command line options" },
#ifdef FEATURE_MODEBUG
    { "-modebug", 1,
      options_set_int_var, (void *)&opt_modebug,
      "LEVEL", "Set debug level" },
#endif
    { NULL, 0, NULL, NULL, NULL, NULL }
};

static const struct cmdline_options_s cmdline_options_common[] = {
    { "-data", 1,
      options_set_datadir, NULL,
      "PATH", "Set data directory" },
    { NULL, 0, NULL, NULL, NULL, NULL }
};

static const struct cmdline_options_s cmdline_options_audio_early[] = {
    { "-audio", 0,
      options_enable_var, (void *)&opt_audio_enabled,
      NULL, "Enable audio" },
    { "-noaudio", 0,
      options_disable_var, (void *)&opt_audio_enabled,
      NULL, "Disable audio" },
    { NULL, 0, NULL, NULL, NULL, NULL }
};

static const struct cmdline_options_s cmdline_options_audio[] = {
    { "-music", 0,
      options_enable_var, (void *)&opt_music_enabled,
      NULL, "Enable music" },
    { "-nomusic", 0,
      options_disable_var, (void *)&opt_music_enabled,
      NULL, "Disable music" },
    { "-sfx", 0,
      options_enable_var, (void *)&opt_sfx_enabled,
      NULL, "Enable SFX" },
    { "-nosfx", 0,
      options_disable_var, (void *)&opt_sfx_enabled,
      NULL, "Disable SFX" },
    { "-musicvol", 1,
      options_set_int_var, (void *)&opt_music_volume,
      "VOLUME", "Set music volume (0..128)" },
    { "-sfxvol", 1,
      options_set_int_var, (void *)&opt_sfx_volume,
      "VOLUME", "Set SFX volume (0..128)" },
    { "-audiohz", 1,
      options_set_int_var, (void *)&opt_audiorate,
      "HZ", "Set audio sample rate (Hz)" },
    { "-audioms", 1,
      options_set_int_var, (void *)&opt_audioslice_ms,
      "MS", "Set max audio slice size (ms)" },
#ifdef HAVE_SAMPLERATE
    { "-libsr", 0,
      options_enable_var, (void *)&opt_use_libsamplerate,
      NULL, "Use libsamplerate" },
    { "-nolibsr", 0,
      options_disable_var, (void *)&opt_use_libsamplerate,
      NULL, "Do not use libsamplerate" },
    { "-libsrscale", 1,
      options_set_int_var, (void *)&opt_libsamplerate_scale,
      "PERCENT", "libsamplerate scaling %" },
    { "-libsrmode", 1,
      options_set_int_var, (void *)&opt_libsamplerate_mode,
      "MODE", "libsamplerate mode (0 = best, 4 = worst)" },
#endif
    { NULL, 0, NULL, NULL, NULL, NULL }
};

/* -------------------------------------------------------------------------- */

static int get_options_w(const struct cmdline_options_s *opts, int lmax)
{
    int i = 0;
    while (opts[i].str != NULL) {
        int l;
        l = strlen(opts[i].str) + 1;
        if (opts[i].str_param) {
            l += strlen(opts[i].str_param);
        }
        if (l > lmax) {
            lmax = l;
        }
        ++i;
    }
    return lmax;
}

static void show_options(const struct cmdline_options_s *opts, int lmax)
{
    int i = 0;
    char fmt1[16];
    char fmt2[16];
    sprintf(fmt1, "    %%-%is ", lmax);
    sprintf(fmt2, "%%s\n%%-%is", lmax + 5);
    while (opts[i].str != NULL) {
        char buf[128];
        if (opts[i].str_help) {
            const char *p, *q;
            sprintf(buf, "%s %s", opts[i].str, opts[i].str_param ? opts[i].str_param : "");
            log_message(fmt1, buf);
            p = opts[i].str_help;
            while ((q = strchr(p, '\n')) != NULL) {
                int len;
                len = q - p;
                memcpy(buf, p, len);
                buf[len] = '\0';
                log_message(fmt2, buf, "");
                p = q + 1;
            }
            log_message("%s\n", p);
        }
        ++i;
    }
}

static int show_usage(char **argv, void *var)
{
    options_show_usage();
    return -1;
}

/* -------------------------------------------------------------------------- */

static const struct cmdline_options_s *find_option_do(const char *name, const struct cmdline_options_s *cmds)
{
    while (cmds->str != NULL) {
        if (strcmp(name, cmds->str) == 0) {
            return cmds;
        }
        ++cmds;
    }

    return NULL;
}

static const struct cmdline_options_s *find_option(const char *name, bool early, bool *was_early)
{
    const struct cmdline_options_s *o = NULL;

    if (0
      || (o = find_option_do(name, cmdline_options_early))
      || (ui_use_audio && (o = find_option_do(name, cmdline_options_audio_early)))
      || (o = find_option_do(name, main_cmdline_options_early))
    ) {
        *was_early = true;
        return o;
    }
    if (0
      || (o = find_option_do(name, cmdline_options_common))
      || (ui_use_audio && (o = find_option_do(name, cmdline_options_audio)))
      || (o = find_option_do(name, os_cmdline_options))
      || (o = find_option_do(name, hw_cmdline_options))
      || (o = find_option_do(name, hw_cmdline_options_extra))
      || (o = find_option_do(name, ui_cmdline_options))
      || (o = find_option_do(name, main_cmdline_options))
    ) {
        *was_early = false;
        return o;
    }

    return NULL;
}

/* -------------------------------------------------------------------------- */

static int options_parse_do(int argc, char **argv, bool early)
{
    int i = 1;
    int num;
    bool was_early;
    const struct cmdline_options_s *o;

    while (i < argc) {
        switch (argv[i][0]) {
            case '+':
            case '-':
                o = find_option(argv[i], early, &was_early);

                if (!o) {
                    log_error("unknown option '%s'\n", argv[i]);
                    return -1;
                }

                num = o->num_param;

                if (num >= (argc - i)) {
                    log_error("option '%s' is missing the parameter\n", argv[i]);
                    return -1;
                }

                if (early == was_early) {
                    if (o->handle(&argv[i], o->var) < 0) {
                        return -1;
                    }
                }

                i += num;
                break;

            default:
                if (!early) {
                    if (main_handle_option(argv[i]) < 0) {
                        log_warning("ignoring unhandled parameter '%s'\n", argv[i]);
                    }
                }
                break;
        }

        ++i;
    }
    return 0;
}

/* -------------------------------------------------------------------------- */

int options_parse_early(int argc, char **argv)
{
    return options_parse_do(argc, argv, true);
}

int options_parse(int argc, char **argv)
{
    return options_parse_do(argc, argv, false);
}

void options_show_usage(void)
{
    int lmax = 0;
    log_message_direct(PACKAGE_NAME " v." PACKAGE_VERSION "\n");
    if (main_usage) {
        main_usage();
    }
    log_message_direct("Options:\n");

    lmax = get_options_w(cmdline_options_early, lmax);
    lmax = get_options_w(cmdline_options_common, lmax);
    if (main_use_lbx) {
    }
    if (ui_use_audio) {
        lmax = get_options_w(cmdline_options_audio_early, lmax);
        lmax = get_options_w(cmdline_options_audio, lmax);
    }
    lmax = get_options_w(os_cmdline_options, lmax);
    lmax = get_options_w(hw_cmdline_options, lmax);
    lmax = get_options_w(hw_cmdline_options_extra, lmax);
    lmax = get_options_w(ui_cmdline_options, lmax);
    lmax = get_options_w(main_cmdline_options_early, lmax);
    lmax = get_options_w(main_cmdline_options, lmax);

    show_options(cmdline_options_early, lmax);
    show_options(cmdline_options_common, lmax);
    if (main_use_lbx) {
    }
    if (ui_use_audio) {
        show_options(cmdline_options_audio_early, lmax);
        show_options(cmdline_options_audio, lmax);
    }
    show_options(os_cmdline_options, lmax);
    show_options(hw_cmdline_options, lmax);
    show_options(hw_cmdline_options_extra, lmax);
    show_options(ui_cmdline_options, lmax);
    show_options(main_cmdline_options_early, lmax);
    show_options(main_cmdline_options, lmax);
}
