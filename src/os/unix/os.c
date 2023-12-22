#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "os.h"
#include "options.h"
#include "lib.h"
#include "util.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

const struct cmdline_options_s os_cmdline_options[] = {
    { NULL, 0, NULL, NULL, NULL, NULL }
};

/* -------------------------------------------------------------------------- */

#define NUM_DATA_PATHS  24
#define NUM_ALL_DATA_PATHS  (NUM_DATA_PATHS + 8)

static char *data_path = NULL;
static char *user_path = NULL;
static char *all_data_paths[NUM_ALL_DATA_PATHS] = { NULL };
static int num_data_paths = 0;

/* -------------------------------------------------------------------------- */

static int os_make_path(const char *path)
{
    if ((path == NULL) || (path[0] == '\0') || ((path[0] == '.') && (path[1] == '\0'))) {
        return 0;
    }
    {
        struct stat st;
        if (stat(path, &st) == 0) {
            return S_ISDIR(st.st_mode) ? 0 : -1;
        }
        return (mkdir(path, 0700) == 0) ? 0 : -1;
    }
}

/* -------------------------------------------------------------------------- */

const char *idstr_os = "unix";

int os_early_init(void)
{
    return 0;
}

int os_init(void)
{
    return 0;
}

void os_shutdown(void)
{
    lib_free(user_path);
    user_path = NULL;
    lib_free(data_path);
    data_path = NULL;
    for (int i = 0; i < num_data_paths; ++i) {
        lib_free(all_data_paths[i]);
        all_data_paths[i] = NULL;
    }
    num_data_paths = 0;
}

const char **os_get_paths_data(void)
{
    if (num_data_paths == 0) {
        const char *p;
        int i = 0;

        if (data_path) {
            all_data_paths[i++] = lib_stralloc(data_path);
        }
        p = getenv("XDG_DATA_HOME");
        if (p && (p[0] != '\0')) {
            all_data_paths[i++] = util_concat(p, "/1oom", NULL);
        }
        p = getenv("XDG_DATA_DIRS");
        if (p && (p[0] != '\0')) {
            char *dirs = lib_stralloc(p);
            char *q = dirs;
            char *tok = dirs;
            while ((*q != '\0') && (i < NUM_DATA_PATHS)) {
                while (*q == ':') {
                    ++q;
                    tok = q;
                }
                while ((*q != ':') && (*q != '\0')) {
                    ++q;
                }
                if (*(q - 1) == '/') {
                    *(q - 1) = '\0';
                }
                {
                    bool flag_done = (*q == '\0');
                    *q = '\0';
                    if (strlen(tok) > 0) {
                        all_data_paths[i++] = util_concat(tok, "/1oom", NULL);
                    }
                    if (!flag_done) {
                        tok = ++q;
                    }
                }
            }
            lib_free(dirs);
        }
        {
            p = getenv("HOME");
            if (p && (p[0] != '\0')) {
                all_data_paths[i++] = util_concat(p, "/.local/share/1oom", NULL);
            }
        }
        all_data_paths[i++] = lib_stralloc("/usr/share/1oom");
        all_data_paths[i++] = lib_stralloc("/usr/local/share/1oom");
        all_data_paths[i++] = lib_stralloc("." FSDEV_DIR_SEP_STR "data");
        all_data_paths[i++] = lib_stralloc(".");
        all_data_paths[i] = NULL;
        num_data_paths = i;
    }
    return (const char **)all_data_paths;
}

const char *os_get_path_data(void)
{
    return data_path;
}

void os_set_path_data(const char *path)
{
    if (data_path) {
        lib_free(data_path);
        data_path = NULL;
    }
    data_path = lib_stralloc(path);
}

void os_set_path_user(const char *path)
{
    if (user_path) {
        lib_free(user_path);
        user_path = NULL;
    }
    user_path = lib_stralloc(path);
}

const char *os_get_path_user(void)
{
    if (user_path == NULL) {
        const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
        if ((xdg_config_home != NULL) && (xdg_config_home[0] != '\0')) {
            user_path = util_concat(xdg_config_home, "/1oom", NULL);
        } else {
            const char *home = getenv("HOME");
            if ((home != NULL) && (home[0] != '\0')) {
                user_path = util_concat(home, "/.config/1oom", NULL);
            } else {
                user_path = lib_stralloc(".");
            }
        }
    }
    return user_path;
}

int os_make_path_user(void)
{
    return os_make_path(os_get_path_user());
}

int os_make_path_for(const char *filename)
{
    int res = 0;
    char *path;
    util_fname_split(filename, &path, NULL);
    if (path != NULL) {
        res = os_make_path(path);
        lib_free(path);
    }
    return res;
}

const char *os_get_fname_save(char *buf, int buflen, int savei/*1..9*/)
{
    return NULL;
}

uint32_t os_get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_usec + tv.tv_sec * 1000000);
}
