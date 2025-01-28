bin_PROGRAMS =

if COMPILE_UICLASSIC
if COMPILE_UICLASSIC_SDL1
bin_PROGRAMS += 1oom_classic_sdl1
endif
endif

if COMPILE_TOOLS
bin_PROGRAMS += 1oom_saveconv
if COMPILE_TOOLS_SDL1
bin_PROGRAMS += 1oom_lbxview_sdl1
endif
endif

SUBDIRS = hw os ui game
DIST_SUBDIRS = hw os ui game

util_sources = \
    bits.h \
    boolvec.h \
    comp.h \
    hw.h \
    lbx.c \
    lbx.h \
    lib.c \
    lib.h \
    log.c \
    log.h \
    main.c \
    main.h \
    options.c \
    options.h \
    os.h \
    rnd.c \
    rnd.h \
    types.h \
    ui.h \
    util.c \
    util.h \
    util_math.c \
    util_math.h

gui_sources = \
    font8x8.c \
    font8x8.h \
    font8x8_draw.c \
    font8x8_draw.h \
    gfxaux.c \
    gfxaux.h \
    kbd.c \
    kbd.h \
    lbxfont.c \
    lbxfont.h \
    lbxgfx.c \
    lbxgfx.h \
    lbxpal.c \
    lbxpal.h \
    mouse.c \
    mouse.h \
    vgabuf.c \
    vgabuf.h \
    vgapal.c \
    vgapal.h

audio_sources = \
    fmt_id.h \
    fmt_mus.c \
    fmt_mus.h \
    fmt_sfx.c \
    fmt_sfx.h

AM_CPPFLAGS = \
    @OS_INCLUDES@ \
    -I$(top_srcdir)/src

common_libs = @OS_LIBS@
game_lib = $(top_builddir)/src/game/libgame.a
gamemain_lib = $(top_builddir)/src/game/libgamemain.a
hwnop_lib = $(top_builddir)/src/hw/nop/libhwnop.a

uiclassic_lib = $(top_builddir)/src/ui/classic/libuiclassic.a
hwsdl1_lib = $(top_builddir)/src/hw/sdl/1/libhwsdl1.a
1oom_classic_sdl1_libs = $(gamemain_lib) $(game_lib) $(uiclassic_lib) $(game_lib) $(hwsdl1_lib) $(common_libs)
1oom_classic_sdl1_SOURCES = $(audio_sources) $(gui_sources) $(util_sources)
1oom_classic_sdl1_LDADD = $(1oom_classic_sdl1_libs) @HW_SDL1_LIBS@ @EXTRA_SDL1_LIBS@ @SAMPLERATE_LIBS@
1oom_classic_sdl1_DEPENDENCIES = $(1oom_classic_sdl1_libs)

uinop_lib = $(top_builddir)/src/ui/nop/libuinop.a
1oom_lbxview_sdl1_libs = $(hwsdl1_lib) $(uinop_lib) $(common_libs)
1oom_lbxview_sdl1_SOURCES = lbxview.c $(audio_sources) $(gui_sources) $(util_sources)
1oom_lbxview_sdl1_LDADD = $(1oom_lbxview_sdl1_libs) @HW_SDL1_LIBS@ @EXTRA_SDL1_LIBS@ @SAMPLERATE_LIBS@
1oom_lbxview_sdl1_DEPENDENCIES = $(1oom_lbxview_sdl1_libs)

1oom_saveconv_libs = $(game_lib) $(hwnop_lib) $(uinop_lib) $(common_libs)
1oom_saveconv_SOURCES = saveconv.c lib.c log.c options.c util.c
1oom_saveconv_LDADD = $(1oom_saveconv_libs)
1oom_saveconv_DEPENDENCIES = $(1oom_saveconv_libs)

.PHONY: libui libhw libos libgame libgamemain

libos:
	(cd os && $(MAKE))
libhw:
	(cd hw && $(MAKE))
libui:
	(cd ui && $(MAKE))
libgame:
	(cd game && $(MAKE) libgame.a)
libgamemain:
	(cd game && $(MAKE) libgamemain.a)
