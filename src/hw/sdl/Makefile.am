AM_CFLAGS=@HW_SDL1_CFLAGS@

AM_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/hw

noinst_LIBRARIES = libhwsdl1.a

libhwsdl1_a_SOURCES = \
    hwsdl1.c \
    hwsdl_audio.c \
    hwsdl_mouse.c \
    hwsdl_opt.c \
    hwsdl_video.c

noinst_HEADERS = \
    hwsdl_mouse.h \
    hwsdl_opt.h

EXTRA_DIST = \
    hwsdl.c
