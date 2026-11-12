AM_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/hw

noinst_LIBRARIES = libhwwin32.a

libhwwin32_a_SOURCES = \
    hwwin32.c \
    hwwin32_audio.c \
    hwwin32_opt.c \
    hwwin32_video.c

noinst_HEADERS = \
    hwwin32.h

libhwwin32_a_CFLAGS = @HW_WIN32_CFLAGS@

