bin_PROGRAMS =
if COMPILE_UICMDLINE
bin_PROGRAMS += 1oom_cmdline
endif

if COMPILE_UICLASSIC
if COMPILE_UICLASSIC_SDL1
bin_PROGRAMS += 1oom_classic_sdl1
endif
if COMPILE_UICLASSIC_SDL2
bin_PROGRAMS += 1oom_classic_sdl2
endif
if COMPILE_UICLASSIC_ALLEG4
bin_PROGRAMS += 1oom_classic_alleg4
endif
endif

if COMPILE_TOOLS
bin_PROGRAMS += 1oom_pbxmake 1oom_saveconv
if COMPILE_TOOLS_SDL1
bin_PROGRAMS += 1oom_lbxview_sdl1
endif
if COMPILE_TOOLS_SDL2
bin_PROGRAMS += 1oom_lbxview_sdl2
endif
if COMPILE_TOOLS_ALLEG4
bin_PROGRAMS += 1oom_lbxview_alleg4
endif
endif

SUBDIRS = hw os ui game
DIST_SUBDIRS = hw os ui game

util_sources = \
    bits.h \
    boolvec.h \
    cfg.c \
    cfg.h \
    comp.h \
    gameapi.h \
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
    pbx.c \
    pbx.h \
    rnd.c \
    rnd.h \
    types.h \
    ui.h \
    util.c \
    util.h \
    util_cstr.c \
    util_cstr.h \
    util_math.c \
    util_math.h \
    version.h

gui_sources = \
    font8x8.c \
    font8x8.h \
    font8x8_draw.c \
    font8x8_draw.h \
    gfxaux.c \
    gfxaux.h \
    gfxlimits.c \
    gfxlimits.h \
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

pic_sources = \
    fmt_pic.c \
    fmt_pic.h

BUILT_SOURCES = version.inc

.PHONY: version.inc.indirect

version.inc: version.inc.indirect

version.inc.indirect:
	$(SHELL) $(srcdir)/version.sh $(top_srcdir) $(srcdir)/version.inc @USE_GIT@ @VERSION_1OOM@

EXTRA_DIST = version.sh

AM_CPPFLAGS = \
    @OS_INCLUDES@ \
    -I$(top_srcdir)/src

common_libs = @OS_LIBS@
game_lib = $(top_builddir)/src/game/libgame.a
gamemain_lib = $(top_builddir)/src/game/libgamemain.a
gamesaveconv_lib = $(top_builddir)/src/game/libgamesaveconv.a

uicmdline_lib = $(top_builddir)/src/ui/cmdline/libuicmdline.a
hwnop_lib = $(top_builddir)/src/hw/nop/libhwnop.a
1oom_cmdline_libs = $(gamemain_lib) $(game_lib) $(uicmdline_lib) $(game_lib) $(hwnop_lib) $(common_libs)
1oom_cmdline_SOURCES = $(util_sources) saveconv.c saveconv.h
1oom_cmdline_LDADD = $(1oom_cmdline_libs) @READLINE_LIBS@
1oom_cmdline_DEPENDENCIES = $(1oom_cmdline_libs)

uiclassic_lib = $(top_builddir)/src/ui/classic/libuiclassic.a
hwsdl1_lib = $(top_builddir)/src/hw/sdl/1/libhwsdl1.a
1oom_classic_sdl1_libs = $(gamemain_lib) $(game_lib) $(uiclassic_lib) $(game_lib) $(hwsdl1_lib) $(common_libs)
1oom_classic_sdl1_SOURCES = $(audio_sources) $(gui_sources) $(util_sources) saveconv.c saveconv.h
1oom_classic_sdl1_LDADD = $(1oom_classic_sdl1_libs) @HW_SDL1_LIBS@ @EXTRA_SDL1_LIBS@ @SAMPLERATE_LIBS@
1oom_classic_sdl1_DEPENDENCIES = $(1oom_classic_sdl1_libs)

uinop_lib = $(top_builddir)/src/ui/nop/libuinop.a
1oom_lbxview_sdl1_libs = $(hwsdl1_lib) $(uinop_lib) $(common_libs)
1oom_lbxview_sdl1_SOURCES = lbxview.c $(audio_sources) $(gui_sources) $(util_sources) $(pic_sources)
1oom_lbxview_sdl1_LDADD = $(1oom_lbxview_sdl1_libs) @HW_SDL1_LIBS@ @EXTRA_SDL1_LIBS@ @SAMPLERATE_LIBS@
1oom_lbxview_sdl1_DEPENDENCIES = $(1oom_lbxview_sdl1_libs)

hwsdl2_lib = $(top_builddir)/src/hw/sdl/2/libhwsdl2.a
1oom_classic_sdl2_libs = $(gamemain_lib) $(game_lib) $(uiclassic_lib) $(game_lib) $(hwsdl2_lib) $(common_libs)
1oom_classic_sdl2_SOURCES = $(audio_sources) $(gui_sources) $(util_sources) saveconv.c saveconv.h
1oom_classic_sdl2_LDADD = $(1oom_classic_sdl2_libs) @HW_SDL2_LIBS@ @EXTRA_SDL2_LIBS@ @SAMPLERATE_LIBS@
1oom_classic_sdl2_DEPENDENCIES = $(1oom_classic_sdl2_libs)

1oom_lbxview_sdl2_libs = $(hwsdl2_lib) $(uinop_lib) $(common_libs)
1oom_lbxview_sdl2_SOURCES = lbxview.c $(audio_sources) $(gui_sources) $(util_sources) $(pic_sources)
1oom_lbxview_sdl2_LDADD = $(1oom_lbxview_sdl2_libs) @HW_SDL2_LIBS@ @EXTRA_SDL2_LIBS@ @SAMPLERATE_LIBS@
1oom_lbxview_sdl2_DEPENDENCIES = $(1oom_lbxview_sdl2_libs)

hwalleg4_lib = $(top_builddir)/src/hw/alleg/4/libhwalleg4.a
1oom_classic_alleg4_libs = $(gamemain_lib) $(game_lib) $(uiclassic_lib) $(game_lib) $(hwalleg4_lib) $(common_libs)
1oom_classic_alleg4_SOURCES = $(audio_sources) $(gui_sources) $(util_sources) saveconv.c saveconv.h
1oom_classic_alleg4_LDADD = $(1oom_classic_alleg4_libs) @HW_ALLEG4_LIBS@ @EXTRA_ALLEG4_LIBS@ @SAMPLERATE_LIBS@
1oom_classic_alleg4_DEPENDENCIES = $(1oom_classic_alleg4_libs)

1oom_lbxview_alleg4_libs = $(hwalleg4_lib) $(uinop_lib) $(common_libs)
1oom_lbxview_alleg4_SOURCES = lbxview.c $(audio_sources) $(gui_sources) $(util_sources) $(pic_sources)
1oom_lbxview_alleg4_LDADD = $(1oom_lbxview_alleg4_libs) @HW_ALLEG4_LIBS@ @EXTRA_ALLEG4_LIBS@ @SAMPLERATE_LIBS@
1oom_lbxview_alleg4_DEPENDENCIES = $(1oom_lbxview_alleg4_libs)

1oom_pbxmake_libs = $(hwnop_lib) $(common_libs)
1oom_pbxmake_SOURCES = pbxmake.c util.c util_cstr.c lib.c log.c
1oom_pbxmake_LDADD = $(1oom_pbxmake_libs)
1oom_pbxmake_DEPENDENCIES = $(1oom_pbxmake_libs)

1oom_saveconv_libs = $(gamesaveconv_lib) $(hwnop_lib) $(uinop_lib) $(common_libs)
1oom_saveconv_SOURCES = saveconv_main.c saveconv_main.h saveconv.c saveconv.h lib.c log.c options.c util.c
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
libgamesaveconv:
	(cd game && $(MAKE) libgamesaveconv.a)
