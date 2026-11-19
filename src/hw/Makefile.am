SUBDIRS = nop
if COMPILE_HW_SDL1
SUBDIRS += sdl
endif
if COMPILE_HW_SDL2
SUBDIRS += sdl2
endif
if COMPILE_HW_SDL3
SUBDIRS += sdl3
endif
if COMPILE_HW_ALLEG4
SUBDIRS += alleg
endif
if COMPILE_HW_X11
SUBDIRS += x11
endif
if COMPILE_HW_WIN32
SUBDIRS += win32
endif

DIST_SUBDIRS = alleg nop sdl sdl2 sdl3 win32 x11

noinst_HEADERS = \
    hw_internal.h
