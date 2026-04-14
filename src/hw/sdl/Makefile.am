SUBDIRS =
if COMPILE_HW_SDL1
SUBDIRS += 1
endif
if COMPILE_HW_SDL2
SUBDIRS += 2
endif
if COMPILE_HW_SDL3
SUBDIRS += 3
endif

DIST_SUBDIRS = 1 2 3

noinst_HEADERS = \
    hwsdl_audio.h \
    hwsdl_opt.h \
    hwsdl_video.h

EXTRA_DIST = \
    hwsdl_opt.c
