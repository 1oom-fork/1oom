AC_INIT([1oom], [0.1])

AC_ARG_ENABLE(modebug, AS_HELP_STRING([--enable-modebug], [add debugging options]))
AC_ARG_WITH(readline, AS_HELP_STRING([--without-readline], [do not try to use the readline library]))
AC_ARG_WITH(samplerate, AS_HELP_STRING([--without-samplerate], [do not try to use libsamplerate]))
AC_ARG_ENABLE(tools, AS_HELP_STRING([--disable-tools], [disables extra tools]))
AC_ARG_ENABLE(uiclassic, AS_HELP_STRING([--disable-uiclassic], [disables classic UI]))
AC_ARG_ENABLE(uicmdline, AS_HELP_STRING([--disable-uicmdline], [disables cmdline UI]))
AC_ARG_ENABLE(sdl1hw, AS_HELP_STRING([--disable-sdl1hw], [disables SDL1 HW]))
AC_ARG_ENABLE(sdl2hw, AS_HELP_STRING([--disable-sdl2hw], [disables SDL2 HW]))
AC_ARG_ENABLE(alleg4hw, AS_HELP_STRING([--disable-alleg4hw], [disables Allegro 4 HW]))

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall foreign])
AM_SILENT_RULES

AC_CANONICAL_HOST

user_cflags=$CFLAGS

AC_PROG_CC_C99
AC_PROG_CPP
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_CHECK_TOOL(RANLIB, ranlib, :)
AC_CHECK_TOOL(STRIP, strip, :)
AC_PROG_INSTALL

AC_C_BIGENDIAN

if test -z "$user_cflags" ; then

    dnl Check for the type of compiler first.
    if test x"$GCC" = "xyes" ; then
        warnings_c="-Wall -std=c99 -pedantic -Wno-inline -Wstrict-prototypes"

        dnl Set appropriate optimization options (better than the default -g -O)
        dnl if using GCC.
        dnl If the user has specified her own `CFLAGS', we do not override them.
        optimization_c="-O3"

        dnl Use -g if available.
        if test x"$ac_cv_prog_cc_g" = "xyes" ; then
            CFLAGS="-g $optimization_c $warnings_c"
        else
            CFLAGS="$optimization_c $warnings_c"
        fi
    fi

    dnl (If not on GCC, just keep the defaults, which are very conservative).
fi

dnl Check whether the options are OK.
AC_PROG_CC_C99


dnl - check common stuff

AC_CHECK_HEADERS(inttypes.h stdbool.h)
AC_CHECK_FUNCS(atexit strcasecmp)


dnl - OS detection

dnl - check for win32 headers

AC_CHECK_HEADERS([wtypes.h winnt.h], [is_win32=yes; AC_DEFINE(IS_WINDOWS,, [Compiling for Windows])], [is_win32=no],
[[#ifdef HAVE_WTYPES_H
#include <wtypes.h>
#endif
#ifdef HAVE_WINNT_H
#include <winnt.h>
#endif
]])

dnl - check for msdos

if test x"$host_vendor" = "xgo32" -o x"$host_vendor" = "xmsdos" -o x"$host_os" = "xmsdosdjgpp"; then
    is_msdos=yes
else
    is_msdos=false
fi


dnl - optional features

if test x"$enable_modebug" = "xyes"; then
    AC_DEFINE(FEATURE_MODEBUG,,[Add debugging options.])
fi

dnl - check for pkg-config
dnl The failing result is "false" for $pkg_config --exists foo to fail.
AC_CHECK_TOOLS(pkg_config, pkg-config, false)

dnl - readline
dnl Check whether we have GNU readline.
dnl The user can force us to not use it with `--without-readline'.
READLINE=""
READLINE_LIBS=
if test x"$with_readline" != "xno"; then
    unset ac_cv_lib_readline_readline
    old_LIBS="$LIBS"
    AC_CHECK_LIB(readline, readline,
               [ READLINE="" READLINE_LIBS="-lreadline"],
               [],,)

    if test "$ac_cv_lib_readline_readline" = "no"; then
        unset ac_cv_lib_readline_readline
        AC_CHECK_LIB(readline, readline,
                   [ READLINE="" READLINE_LIBS="-lreadline -ltermcap"],
                   [], [-ltermcap],)
    fi

    if test "$ac_cv_lib_readline_readline" = "no"; then
        unset ac_cv_lib_readline_readline
        AC_CHECK_LIB(readline, readline,
                   [ READLINE="" READLINE_LIBS="-lreadline -lncurses"],
                   [], [-lncurses],)
    fi

    if test "$ac_cv_lib_readline_readline" = "yes"; then
        AC_CHECK_HEADERS(readline/readline.h, [AC_DEFINE(HAVE_READLINE,, [readline available])])
        AC_CHECK_LIB(readline, rl_readline_name,
                 [ AC_DEFINE(HAVE_RLNAME,,
                 [Does the `readline' library support `rl_readline_name'?]) ],,)
    fi
    LIBS="${old_LIBS}"
fi
AC_SUBST(READLINE)
AC_SUBST(READLINE_LIBS)


dnl - libsamplerate

SAMPLERATE_LIBS=
AS_IF([test x"$with_samplerate" != "xno"], [
    AC_CHECK_LIB(samplerate, src_new, [have_samplerate=yes; SAMPLERATE_LIBS="-lsamplerate"], [
        AS_IF([test "x$with_samplerate" = "xyes"], [AC_MSG_FAILURE(
            [--with-samplerate was given, but test for libsamplerate failed])
        ])
    ])
])
AC_SUBST(SAMPLERATE_LIBS)

if test x"$have_samplerate" = "xyes"; then
    AC_DEFINE(HAVE_SAMPLERATE,,[Enable libsamplerate])
fi


dnl - SDL1
dnl Check and setup SDL 1.2 compilation.

AM_CONDITIONAL(COMPILE_HW_SDL, false)
AM_CONDITIONAL(COMPILE_HW_SDL1, false)
HW_SDL1_CFLAGS=
HW_SDL1_LIBS=
if test x"$enable_sdl1hw" != "xno"; then
    dnl check for sdl-config
    AC_CHECK_TOOLS(sdl_config, sdl-config, no)

    old_cflags=$CFLAGS
    old_LIBS="$LIBS"
    old_sdl_libs=$HW_SDL1_LIBS

    dnl First check for user supplied libs/cflags.
    dnl In none are given, try sdl-config.
    dnl If no sdl-config was found, try pkg-config.
    dnl If this fails, we assume that there are no extra CFLAGS
    dnl and that libSDL can be found in the default lib search path.
    if test x"$SDL1_LIBS" != "x" -o x"$SDL1_CFLAGS" != "x"; then
        HW_SDL1_CFLAGS="$CFLAGS $SDL1_CFLAGS"
        HW_SDL1_LIBS="$SDL1_LIBS"
    elif test x"$sdl_config" != "xno"; then
        HW_SDL1_CFLAGS="$CFLAGS `$sdl_config --cflags`"
        HW_SDL1_LIBS="`$sdl_config --libs`"
    elif $pkg_config --exists sdl; then
        HW_SDL1_CFLAGS="$CFLAGS `$pkg_config --cflags sdl`"
        HW_SDL1_LIBS="`$pkg_config --libs sdl`"
    else
        HW_SDL1_LIBS="-lSDL"
    fi

    CFLAGS="$HW_SDL1_CFLAGS"
    LIBS="$HW_SDL1_LIBS"

    dnl check for the SDL.h header
    AC_CHECK_HEADERS(SDL.h,[AC_CHECK_LIB(SDL, SDL_SetVideoMode,
                     [],
                     [CFLAGS=$old_cflags; break],$EXTRA_SDL1_LIBS)],
                     [CFLAGS=$old_cflags; break],$EXTRA_SDL1_LIBS)
    LIBS="$old_LIBS"

    if test x"$ac_cv_lib_SDL_SDL_SetVideoMode" != "xyes"; then
        AC_MSG_WARN([SDL1 headers and/or libraries missing!])
    else
        AC_DEFINE(USE_SDL1HW,,[Enable SDL1 HW support.])
        AM_CONDITIONAL(COMPILE_HW_SDL1, true)
        AM_CONDITIONAL(COMPILE_HW_SDL, true)
        have_sdl1=yes

        dnl check for SDL_mixer
        AC_MSG_CHECKING(for the SDL_mixer library)

        old_sdl_libs="$HW_SDL1_LIBS"
        if test x"$SDL1MIXER_LIBS" != "x"; then
            HW_SDL1_LIBS="$HW_SDL1_LIBS $SDL1MIXER_LIBS"
        elif $pkg_config --exists SDL_mixer; then
            HW_SDL1_LIBS="`$pkg_config --libs SDL_mixer`"
        else
            HW_SDL1_LIBS="$HW_SDL1_LIBS -lSDL_mixer"
        fi
        LIBS="$old_LIBS $HW_SDL1_LIBS"

        AC_TRY_LINK([#include "SDL_mixer.h"],
                    [Mix_LoadMUS(0)],
                    [AC_MSG_RESULT(yes);
                     have_sdl1mixer=yes],
                    [AC_MSG_RESULT(no);
                     HW_SDL1_LIBS="$old_sdl_libs"])

        LIBS="$old_LIBS"

        if test x"$have_sdl1mixer" = "xyes"; then
            AC_DEFINE(HAVE_SDL1MIXER,,[Enable SDL_mixer])
        else
            AC_MSG_WARN([SDL_mixer not found, audio disabled!])
        fi

        dnl check for OpenGL libs
        AC_MSG_CHECKING(for the OpenGL library)

        old_sdl_libs="$HW_SDL1_LIBS"
        HW_SDL1_LIBS="$HW_SDL1_LIBS -lGL"
        LIBS="$old_LIBS $HW_SDL1_LIBS"

        AC_TRY_LINK([#include <SDL_opengl.h>],
                    [glViewport(1,2,3,4)],
                    [AC_MSG_RESULT(yes);
                     have_sdl1gl_lib=yes],
                    [AC_MSG_RESULT(no);
                     HW_SDL1_LIBS="$old_sdl_libs"])

        LIBS="$old_LIBS"

        if test x"$have_sdl1gl_lib" != "xyes"; then
            AC_MSG_CHECKING(whether we can use the opengl32 library)
            old_sdl_libs="$HW_SDL1_LIBS"
            HW_SDL1_LIBS="$HW_SDL1_LIBS -lopengl32"
            LIBS="$LIBS $HW_SDL1_LIBS"
            AC_TRY_LINK([#include "SDL_opengl.h"],
                        [glViewport(1,2,3,4)],
                        [AC_MSG_RESULT(yes);
                         have_sdl1gl_lib=yes],
                        [AC_MSG_RESULT(no);
                         HW_SDL1_LIBS="$old_sdl_libs"])
            LIBS="$old_LIBS"
        fi

        if test x"$have_sdl1gl_lib" = "xyes"; then
            AC_DEFINE(HAVE_SDL1GL,,[Enable SDL1 OpenGL])
        else
            AC_MSG_WARN([SDL1 OpenGL not available, graphics resizing disabled!])
        fi

        AC_MSG_CHECKING(for the SDLmain library)
        old_sdl_libs="$HW_SDL1_LIBS"
        HW_SDL1_LIBS="$HW_SDL1_LIBS -lSDLmain -lSDL"
        LIBS="$LIBS $HW_SDL1_LIBS"
        AC_TRY_LINK([#include "SDL_main.h"
                     int main(int argc, char *argv[])
                     { return 0; }
                     #undef main
                     #define main something_else],
                    [return 0;],
                    [AC_MSG_RESULT(yes);
                     have_sdlmain=yes],
                    [AC_MSG_RESULT(no);
                     HW_SDL1_LIBS="$old_sdl_libs"])
        LIBS="$old_LIBS"

        if test x"$have_sdlmain" != "xyes"; then
            AC_MSG_CHECKING(whether we can use the SDLmain library when adding -lmingw32)
            old_sdl_libs="$HW_SDL1_LIBS"
            HW_SDL1_LIBS="$HW_SDL1_LIBS -lmingw32 -lSDLmain -lSDL"
            LIBS="$LIBS $HW_SDL1_LIBS"
            AC_TRY_LINK([#include "SDL_main.h"
                         int main(int argc, char *argv[])
                         { return 0; }
                         #undef main
                         #define main something_else],
                        [return 0;],
                        [AC_MSG_RESULT(yes);
                         have_sdlmain=yes],
                        [AC_MSG_RESULT(no);
                         HW_SDL1_LIBS="$old_sdl_libs"])
            LIBS="$old_LIBS"
        fi

        if test x"$have_sdlmain" = "xyes"; then
            AC_DEFINE(HAVE_SDLMAIN,,[Enable SDLmain replacement])
        fi
    fi
    CFLAGS="$old_cflags"
    LIBS="$old_LIBS"
fi
AC_SUBST(HW_SDL1_CFLAGS)
AC_SUBST(HW_SDL1_LIBS)


dnl - SDL2
dnl Check and setup SDL 2 compilation.

AM_CONDITIONAL(COMPILE_HW_SDL2, false)
HW_SDL2_CFLAGS=
HW_SDL2_LIBS=
if test x"$enable_sdl2hw" != "xno"; then
    dnl check for sdl2-config
    AC_CHECK_TOOLS(sdl2_config, sdl2-config, no)

    old_cflags=$CFLAGS
    old_LIBS="$LIBS"
    old_sdl_libs=$HW_SDL2_LIBS

    dnl First check for user supplied libs/cflags.
    dnl In none are given, try sdl2-config.
    dnl If no sdl2-config was found, try pkg-config.
    dnl If this fails, we assume that there are no extra CFLAGS
    dnl and that libSDL2 can be found in the default lib search path.
    if test x"$SDL2_LIBS" != "x" -o x"$SDL2_CFLAGS" != "x"; then
        HW_SDL2_CFLAGS="$CFLAGS $SDL2_CFLAGS"
        HW_SDL2_LIBS="$SDL2_LIBS"
    elif test x"$sdl2_config" != "xno"; then
        HW_SDL2_CFLAGS="$CFLAGS `$sdl2_config --cflags`"
        HW_SDL2_LIBS="`$sdl2_config --libs`"
    elif $pkg_config --exists sdl2; then
        HW_SDL2_CFLAGS="$CFLAGS `$pkg_config --cflags sdl2`"
        HW_SDL2_LIBS="`$pkg_config --libs sdl2`"
    else
        HW_SDL2_LIBS="-lSDL2"
    fi

    CFLAGS="$HW_SDL2_CFLAGS"
    LIBS="$HW_SDL2_LIBS"

    dnl check for the SDL.h header
    AC_CHECK_HEADERS(SDL.h,[AC_CHECK_LIB(SDL2, SDL_CreateTexture,
                     [],
                     [CFLAGS=$old_cflags; break],$EXTRA_SDL2_LIBS)],
                     [CFLAGS=$old_cflags; break],$EXTRA_SDL2_LIBS)
    LIBS="$old_LIBS"

    if test x"$ac_cv_lib_SDL2_SDL_CreateTexture" != "xyes"; then
        AC_MSG_WARN([SDL2 headers and/or libraries missing!])
    else
        AC_DEFINE(USE_SDL2HW,,[Enable SDL2 HW support.])
        AM_CONDITIONAL(COMPILE_HW_SDL2, true)
        AM_CONDITIONAL(COMPILE_HW_SDL, true)
        have_sdl2=yes

        dnl check for SDL2_mixer
        AC_MSG_CHECKING(for the SDL2_mixer library)

        old_sdl_libs="$HW_SDL2_LIBS"
        if test x"$SDL2MIXER_LIBS" != "x"; then
            HW_SDL2_LIBS="$HW_SDL2_LIBS $SDL2MIXER_LIBS"
        elif $pkg_config --exists SDL2_mixer; then
            HW_SDL2_LIBS="`$pkg_config --libs SDL2_mixer`"
        else
            HW_SDL2_LIBS="$HW_SDL2_LIBS -lSDL2_mixer"
        fi
        LIBS="$old_LIBS $HW_SDL2_LIBS"

        AC_TRY_LINK([#include "SDL_mixer.h"],
                    [Mix_LoadMUS(0)],
                    [AC_MSG_RESULT(yes);
                     have_sdl2mixer=yes],
                    [AC_MSG_RESULT(no);
                     HW_SDL2_LIBS="$old_sdl_libs"])

        LIBS="$old_LIBS"

        if test x"$have_sdl2mixer" = "xyes"; then
            AC_DEFINE(HAVE_SDL2MIXER,,[Enable SDL2_mixer])
        else
            AC_MSG_WARN([SDL2_mixer not found, audio disabled!])
        fi

        AC_MSG_CHECKING(for the SDL2main library)
        old_sdl_libs="$HW_SDL2_LIBS"
        HW_SDL2_LIBS="$HW_SDL2_LIBS -lSDL2main -lSDL2"
        LIBS="$LIBS $HW_SDL2_LIBS"
        AC_TRY_LINK([#include "SDL_main.h"
                     int main(int argc, char *argv[])
                     { return 0; }
                     #undef main
                     #define main something_else],
                    [return 0;],
                    [AC_MSG_RESULT(yes);
                     have_sdl2main=yes],
                    [AC_MSG_RESULT(no);
                     HW_SDL2_LIBS="$old_sdl_libs"])
        LIBS="$old_LIBS"

        if test x"$have_sdl2main" = "xyes"; then
            AC_DEFINE(HAVE_SDL2MAIN,,[Enable SDL2main replacement])
        fi
    fi
    CFLAGS="$old_cflags"
    LIBS="$old_LIBS"
fi
AC_SUBST(HW_SDL2_CFLAGS)
AC_SUBST(HW_SDL2_LIBS)


dnl - Allegro 4
dnl Check and setup Allegro 4 compilation.

AM_CONDITIONAL(COMPILE_HW_ALLEG4, false)
HW_ALLEG4_LIBS=
if test x"$enable_alleg4hw" != "xno"; then
    old_LIBS="$LIBS"

    dnl check for the allegro.h header
    AC_CHECK_HEADERS(allegro.h,[AC_CHECK_LIB(alleg, allegro_exit,
                     [],
                     [break],$EXTRA_ALLEG4_LIBS)],
                     [break],$EXTRA_ALLEG4_LIBS)

    if test x"$ac_cv_lib_alleg_allegro_exit" != "xyes"; then
        AC_MSG_WARN([Allegro 4 headers and/or libraries missing!])
    else
        AC_MSG_CHECKING(for Allegro version 4)
        AC_TRY_LINK([#include "allegro.h"
                    #if ALLEGRO_VERSION != 4
                    #error "not version 4"
                    #endif
                    ],
                [return 0;],
                [AC_MSG_RESULT(yes);
                 have_alleg4=yes],
                [AC_MSG_RESULT(no);
                 have_alleg4=no])

        if test x"$have_alleg4" = "xyes"; then
            AC_DEFINE(USE_ALLEG4HW,,[Enable Allegro 4 HW support.])
            AM_CONDITIONAL(COMPILE_HW_ALLEG4, true)
            HW_ALLEG4_LIBS="-lalleg"
        else
            AC_MSG_WARN([Allegro is not version 4!])
        fi
    fi

    LIBS="$old_LIBS"
fi
AC_SUBST(HW_ALLEG4_LIBS)


dnl - setup the system-specific object files

if test x"$is_win32" = "xyes"; then
    OS_DIR="\$(top_builddir)/src/os/win32"
    OS_SRC_DIR="\$(top_srcdir)/src/os/win32"
    BINDIST_SCRIPT="\$(top_srcdir)/make-bindist_win32.sh"
    AM_CONDITIONAL(COMPILE_UNIX, false)
    AM_CONDITIONAL(COMPILE_WIN32, true)
    AM_CONDITIONAL(COMPILE_MSDOS, false)
elif test x"$is_msdos" = "xyes"; then
    OS_DIR="\$(top_builddir)/src/os/msdos"
    OS_SRC_DIR="\$(top_srcdir)/src/os/msdos"
    BINDIST_SCRIPT="\$(top_srcdir)/make-bindist_msdos.sh"
    AM_CONDITIONAL(COMPILE_UNIX, false)
    AM_CONDITIONAL(COMPILE_WIN32, false)
    AM_CONDITIONAL(COMPILE_MSDOS, true)
else
    OS_DIR="\$(top_builddir)/src/os/unix"
    OS_SRC_DIR="\$(top_srcdir)/src/os/unix"
    BINDIST_SCRIPT="\$(top_srcdir)/make-bindist_install.sh"
    AM_CONDITIONAL(COMPILE_UNIX, true)
    AM_CONDITIONAL(COMPILE_WIN32, false)
    AM_CONDITIONAL(COMPILE_MSDOS, false)
fi
OS_LIBS="$OS_DIR/libos.a"
OS_INCLUDES="-I$OS_SRC_DIR"


dnl - build exe opions

if test x"$enable_uicmdline" != "xno"; then
    AM_CONDITIONAL(COMPILE_UICMDLINE, true)
else
    AM_CONDITIONAL(COMPILE_UICMDLINE, false)
fi

if test x"$enable_uiclassic" != "xno"; then
    AM_CONDITIONAL(COMPILE_UICLASSIC, true)
    if test x"$have_sdl1" = "xyes"; then
        AM_CONDITIONAL(COMPILE_UICLASSIC_SDL1, true)
        have_classic_hw=yes
    else
        AM_CONDITIONAL(COMPILE_UICLASSIC_SDL1, false)
    fi
    if test x"$have_sdl2" = "xyes"; then
        AM_CONDITIONAL(COMPILE_UICLASSIC_SDL2, true)
        have_classic_hw=yes
    else
        AM_CONDITIONAL(COMPILE_UICLASSIC_SDL2, false)
    fi
    if test x"$have_alleg4" = "xyes"; then
        AM_CONDITIONAL(COMPILE_UICLASSIC_ALLEG4, true)
        have_classic_hw=yes
    else
        AM_CONDITIONAL(COMPILE_UICLASSIC_ALLEG4, false)
    fi
else
    AM_CONDITIONAL(COMPILE_UICLASSIC, false)
    AM_CONDITIONAL(COMPILE_UICLASSIC_SDL1, false)
    AM_CONDITIONAL(COMPILE_UICLASSIC_SDL2, false)
    AM_CONDITIONAL(COMPILE_UICLASSIC_ALLEG4, false)
fi

if test x"$enable_tools" != "xno"; then
    AM_CONDITIONAL(COMPILE_TOOLS, true)
    if test x"$have_sdl1" = "xyes"; then
        AM_CONDITIONAL(COMPILE_TOOLS_SDL1, true)
    else
        AM_CONDITIONAL(COMPILE_TOOLS_SDL1, false)
    fi
    if test x"$have_sdl2" = "xyes"; then
        AM_CONDITIONAL(COMPILE_TOOLS_SDL2, true)
    else
        AM_CONDITIONAL(COMPILE_TOOLS_SDL2, false)
    fi
    if test x"$have_alleg4" = "xyes"; then
        AM_CONDITIONAL(COMPILE_TOOLS_ALLEG4, true)
    else
        AM_CONDITIONAL(COMPILE_TOOLS_ALLEG4, false)
    fi
else
    AM_CONDITIONAL(COMPILE_TOOLS, false)
    AM_CONDITIONAL(COMPILE_TOOLS_SDL1, false)
    AM_CONDITIONAL(COMPILE_TOOLS_SDL2, false)
    AM_CONDITIONAL(COMPILE_TOOLS_ALLEG4, false)
fi

if test x"$enable_uiclassic" != "xno" -a x"$have_classic_hw" != "xyes"; then
    AC_MSG_ERROR([classic UI enabled but no suitable HW library found!])
fi

dnl - finish up

AC_SUBST(HAVE_READLINE)
AC_SUBST(EXTRA_SDL1_LIBS)
AC_SUBST(EXTRA_SDL2_LIBS)
AC_SUBST(EXTRA_ALLEG4_LIBS)
AC_SUBST(OS_INCLUDES)
AC_SUBST(OS_LIBS)
AC_SUBST(OS_DIR)
AC_SUBST(OS_SRC_DIR)
AC_SUBST(BINDIST_SCRIPT)
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 doc/ext/Makefile
                 src/Makefile
                 src/game/Makefile
                 src/os/Makefile
                 src/os/unix/Makefile
                 src/os/win32/Makefile
                 src/os/msdos/Makefile
                 src/ui/Makefile
                 src/ui/cmdline/Makefile
                 src/ui/classic/Makefile
                 src/ui/nop/Makefile
                 src/hw/Makefile
                 src/hw/nop/Makefile
                 src/hw/sdl/Makefile
                 src/hw/sdl/1/Makefile
                 src/hw/sdl/2/Makefile
                 src/hw/alleg/Makefile
                 src/hw/alleg/4/Makefile
                ])
AC_OUTPUT
