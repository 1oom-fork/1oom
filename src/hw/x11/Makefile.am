AM_CFLAGS=@HW_X11_CFLAGS@

AM_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/hw

noinst_LIBRARIES = libx11.a

libx11_a_SOURCES = \
    hwx11_audio.c \
    hwx11_opt.c \
    hwx11_video.c \
    hwx11.c

noinst_HEADERS = \
    hwx11_video.h
