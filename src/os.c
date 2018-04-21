#include "config.h"

#include "lib.h"
#include "os.h"
#include "util.h"

#ifdef IS_WINDOWS
#include <windows.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#endif

/* -------------------------------------------------------------------------- */

#ifdef IS_WINDOWS
const char *idstr_os = "win32";
#else
const char *idstr_os = "unix";
#endif

/* -------------------------------------------------------------------------- */

static int os_make_path(const char *path)
{
    if ((path == NULL) || ((path[0] == '.') && (path[1] == '\0'))) {
        return 0;
    }
#ifdef IS_WINDOWS
    return mkdir(path);
#else
    if (access(path, F_OK)) {
        return mkdir(path, 0700);
    }
#endif
    return 0;
}

/* -------------------------------------------------------------------------- */

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

uint32_t os_get_time_us(void)
{
#ifdef IS_WINDOWS
    return GetTickCount() * 1000;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_usec + tv.tv_sec * 1000000);
#endif
}
