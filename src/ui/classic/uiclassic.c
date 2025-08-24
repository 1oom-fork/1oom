#include "config.h"

#include <stdio.h>
#include <string.h>

#include "ui.h"
#include "bits.h"
#include "gfxaux.h"
#include "hw.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lbxpal.h"
#include "lib.h"
#include "log.h"
#include "mouse.h"
#include "options.h"
#include "os.h"
#include "types.h"
#include "uidefs.h"
#include "uifix.h"
#include "uipal.h"
#include "uiobj.h"

/* -------------------------------------------------------------------------- */

const struct cmdline_options_s ui_cmdline_options[] = {
    { "-uinodelay", 0,
      options_disable_bool_var, (void *)&ui_delay_enabled,
      NULL, "Disable UI delay" },
    { "-uifixbugs", 0,
      options_enable_bool_var, (void *)&ui_fix_bugs,
      NULL, "Fix UI bugs" },
    { "-uifixqol", 0,
      options_enable_bool_var, (void *)&ui_fix_qol,
      NULL, "Fix QOL (UI)" },
    { NULL, 0, NULL, NULL, NULL, NULL }
};

/* -------------------------------------------------------------------------- */

const char *idstr_ui = "classic";

struct ui_data_s ui_data = { 0 };

bool ui_fix_bugs = false;
bool ui_fix_qol = false;

bool ui_use_audio = true;

/* -------------------------------------------------------------------------- */

static void init_lbx_design(void)
{
    ui_data.gfx.design.blank = lbxfile_item_get(LBXFILE_DESIGN, 1, 0);
    ui_data.gfx.design.icon_up = lbxfile_item_get(LBXFILE_DESIGN, 3, 0);
    ui_data.gfx.design.icon_dn = lbxfile_item_get(LBXFILE_DESIGN, 2, 0);
    ui_data.gfx.design.count_up = lbxfile_item_get(LBXFILE_DESIGN, 4, 0);
    ui_data.gfx.design.count_dn = lbxfile_item_get(LBXFILE_DESIGN, 5, 0);
    ui_data.gfx.design.bg = lbxfile_item_get(LBXFILE_DESIGN, 0, 0);
    ui_data.gfx.design.pop1_ul = lbxfile_item_get(LBXFILE_DESIGN, 6, 0);
    ui_data.gfx.design.pop1_ur = lbxfile_item_get(LBXFILE_DESIGN, 7, 0);
    ui_data.gfx.design.pop1_dl = lbxfile_item_get(LBXFILE_DESIGN, 9, 0);
    ui_data.gfx.design.pop1_dr = lbxfile_item_get(LBXFILE_PLANETS, 0x31, 0);
    ui_data.gfx.design.titlebox = lbxfile_item_get(LBXFILE_DESIGN, 8, 0);
    ui_data.gfx.design.popscrol_u = lbxfile_item_get(LBXFILE_DESIGN, 0xb, 0);
    ui_data.gfx.design.popscrol_d = lbxfile_item_get(LBXFILE_DESIGN, 0xc, 0);
}

static void init_lbx_space(void)
{
    for (int i = 0; i < 5; ++i) {
        ui_data.gfx.space.bg[i] = lbxfile_item_get(LBXFILE_SPACE, 0x1a + i, 0);
    }
    ui_data.gfx.space.box = lbxfile_item_get(LBXFILE_SPACE, 1, 0);
    ui_data.gfx.space.box_x = lbxfile_item_get(LBXFILE_SPACE, 0x32, 0);
    ui_data.gfx.space.box_y = lbxfile_item_get(LBXFILE_SPACE, 0x33, 0);
    ui_data.gfx.space.box_xy = lbxfile_item_get(LBXFILE_SPACE, 0x34, 0);
    ui_data.gfx.space.done = lbxfile_item_get(LBXFILE_SPACE, 3, 0);
    ui_data.gfx.space.retreat = lbxfile_item_get(LBXFILE_SPACE, 5, 0);
    ui_data.gfx.space.retr_off = lbxfile_item_get(LBXFILE_SPACE, 2, 0);
    ui_data.gfx.space.wait = lbxfile_item_get(LBXFILE_SPACE, 6, 0);
    ui_data.gfx.space.autob = lbxfile_item_get(LBXFILE_SPACE, 0x11, 0);
    ui_data.gfx.space.special = lbxfile_item_get(LBXFILE_SPACE, 0x12, 0);
    ui_data.gfx.space.spec_off = lbxfile_item_get(LBXFILE_SPACE, 0x20, 0);
    ui_data.gfx.space.scan = lbxfile_item_get(LBXFILE_SPACE, 0x15, 0);
    ui_data.gfx.space.scan_off = lbxfile_item_get(LBXFILE_SPACE, 0x14, 0);
    ui_data.gfx.space.planet = lbxfile_item_get(LBXFILE_SPACE, 0x16, 0);
    ui_data.gfx.space.planet_off = lbxfile_item_get(LBXFILE_SPACE, 0x1f, 0);
    for (int i = 0; i < 10; ++i) {
        ui_data.gfx.space.explos[i] = lbxfile_item_get(LBXFILE_SPACE, 7 + i, 0);
    }
    ui_data.gfx.space.warp1 = lbxfile_item_get(LBXFILE_SPACE, 0x17, 0);
    ui_data.gfx.space.warp2 = lbxfile_item_get(LBXFILE_SPACE, 0x2d, 0);
    ui_data.gfx.space.warp3 = lbxfile_item_get(LBXFILE_SPACE, 0x2e, 0);
    ui_data.gfx.space.warp4 = lbxfile_item_get(LBXFILE_SPACE, 0x2f, 0);
    ui_data.gfx.space.technull = lbxfile_item_get(LBXFILE_SPACE, 0x18, 0);
    ui_data.gfx.space.misbutt = lbxfile_item_get(LBXFILE_SPACE, 0x19, 0);
    ui_data.gfx.space.misl_off = lbxfile_item_get(LBXFILE_SPACE, 4, 0);
    ui_data.gfx.space.warpout = lbxfile_item_get(LBXFILE_SPACE, 0x21, 0);
    ui_data.gfx.space.envterm = lbxfile_item_get(LBXFILE_SPACE, 0x22, 0);
    ui_data.gfx.space.enviro = lbxfile_item_get(LBXFILE_SPACE, 0x13, 0);
    ui_data.gfx.space.base_btn = lbxfile_item_get(LBXFILE_SPACE, 0x23, 0);
    ui_data.gfx.space.dis_bem2 = lbxfile_item_get(LBXFILE_SPACE, 0x24, 0);
    ui_data.gfx.space.stasis2 = lbxfile_item_get(LBXFILE_SPACE, 0x25, 0);
    ui_data.gfx.space.vs2 = lbxfile_item_get(LBXFILE_SPACE, 0x27, 0);
    ui_data.gfx.space.vp2_top = lbxfile_item_get(LBXFILE_SPACE, 0x2a, 0);
    ui_data.gfx.space.vp2_data = lbxfile_item_get(LBXFILE_SPACE, 0x2b, 0);
    ui_data.gfx.space.vp2_line = lbxfile_item_get(LBXFILE_SPACE, 0x2c, 0);
    ui_data.gfx.space.vp2_bottom = lbxfile_item_get(LBXFILE_SPACE, 0x39, 0);
    ui_data.gfx.space.blk_hole = lbxfile_item_get(LBXFILE_SPACE, 0x26, 0);
    ui_data.gfx.space.bombs = lbxfile_item_get(LBXFILE_SPACE, 0x30, 0);
    ui_data.gfx.space.biologic = lbxfile_item_get(LBXFILE_SPACE, 0x31, 0);
    ui_data.gfx.space.circle = lbxfile_item_get(LBXFILE_SPACE, 0x35, 0);
    ui_data.gfx.space.sphere2 = lbxfile_item_get(LBXFILE_SPACE, 0x36, 0);
    ui_data.gfx.space.asteroid[0] = lbxfile_item_get(LBXFILE_SPACE, 0x28, 0);
    ui_data.gfx.space.asteroid[1] = lbxfile_item_get(LBXFILE_SPACE, 0x29, 0);
    ui_data.gfx.space.asteroid[2] = lbxfile_item_get(LBXFILE_SPACE, 0x37, 0);
    ui_data.gfx.space.asteroid[3] = lbxfile_item_get(LBXFILE_SPACE, 0x38, 0);
    for (int i = 0; i < 8; ++i) {
        ui_data.gfx.missile.missiles[i] = lbxfile_item_get(LBXFILE_MISSILE, i, 0);
        ui_data.gfx.missile.antimatr[i] = lbxfile_item_get(LBXFILE_MISSILE, 0x8 + i, 0);
        ui_data.gfx.missile.hellfire[i] = lbxfile_item_get(LBXFILE_MISSILE, 0x10 + i, 0);
        ui_data.gfx.missile.proton[i] = lbxfile_item_get(LBXFILE_MISSILE, 0x18 + i, 0);
        ui_data.gfx.missile.plasmaqt[i] = lbxfile_item_get(LBXFILE_MISSILE, 0x20 + i, 0);
    }
}

static void init_lbx_news(void)
{
    ui_data.gfx.news.tv = lbxfile_item_get(LBXFILE_NEWSCAST, 0, 0);
    ui_data.gfx.news.gnn = lbxfile_item_get(LBXFILE_NEWSCAST, 1, 0);
    ui_data.gfx.news.nc = lbxfile_item_get(LBXFILE_NEWSCAST, 2, 0);
    ui_data.gfx.news.world = lbxfile_item_get(LBXFILE_NEWSCAST, 3, 0);
    ui_data.gfx.news.icon = 0;
}

static void init_gfx(void)
{
    for (int i = 0; i < NEBULA_MAX; ++i) {
        ui_data.gfx.starmap.nebula[i] = NULL;
        ui_data.gfx.starmap.smnebula[i] = NULL;
    }
    ui_data.gfx.starmap.mainview = lbxfile_item_get(LBXFILE_STARMAP, 0, 0);
    ui_data.gfx.starmap.starback = lbxfile_item_get(LBXFILE_STARMAP, 1, 0);
    ui_data.gfx.starmap.starbak2 = lbxfile_item_get(LBXFILE_STARMAP, 2, 0);
    for (int i = 0; i < 12; ++i) {
        const int jtbl[12] = { 0, 1, 2, 4, 3, 5, 6, 7, 8, 10, 9, 11 };
        ui_data.gfx.starmap.stars[jtbl[i]] = lbxfile_item_get(LBXFILE_STARMAP, 3 + i, 0);
    }
    ui_data.gfx.starmap.planbord = lbxfile_item_get(LBXFILE_STARMAP, 0x19, 0);
    ui_data.gfx.starmap.yourplnt = lbxfile_item_get(LBXFILE_STARMAP, 0x1a, 0);
    ui_data.gfx.starmap.unexplor = lbxfile_item_get(LBXFILE_STARMAP, 0x1b, 0);
    ui_data.gfx.starmap.en_colny = lbxfile_item_get(LBXFILE_STARMAP, 0x1c, 0);
    ui_data.gfx.starmap.no_colny = lbxfile_item_get(LBXFILE_STARMAP, 0x1d, 0);
    ui_data.gfx.starmap.col_butt_ship = lbxfile_item_get(LBXFILE_STARMAP, 0x1e, 0);
    ui_data.gfx.starmap.col_butt_reloc = lbxfile_item_get(LBXFILE_STARMAP, 0x1f, 0);
    ui_data.gfx.starmap.col_butt_trans = lbxfile_item_get(LBXFILE_STARMAP, 0x20, 0);
    ui_data.gfx.starmap.sky = lbxfile_item_get(LBXFILE_STARMAP, 0x2f, 0);
    for (int i = 0; i < 6; ++i) {
        const int jtbl[6] = { 0, 1, 2, 4, 3, 5 };
        ui_data.gfx.starmap.smstars[jtbl[i]] = lbxfile_item_get(LBXFILE_STARMAP, 0x21 + i, 0);
    }
    for (int i = 0; i < 6; ++i) {
        ui_data.gfx.starmap.smalflag[i] = lbxfile_item_get(LBXFILE_STARMAP, 0x27 + i, 0);
    }
    ui_data.gfx.starmap.stargate = lbxfile_item_get(LBXFILE_STARMAP, 0x2d, 0);
    ui_data.gfx.starmap.smallstr = lbxfile_item_get(LBXFILE_STARMAP, 0x2e, 0);
    for (int i = 0; i < 0xa; ++i) {
        ui_data.gfx.starmap.smneb[i] = lbxfile_item_get(LBXFILE_STARMAP, 0x35 + i, 0);
    }
    for (int i = 0; i < 0x1e; ++i) {
        ui_data.gfx.starmap.smneb[0xa + i] = lbxfile_item_get(LBXFILE_NEBULA, i, 0);
    }
    ui_data.gfx.starmap.relocate = lbxfile_item_get(LBXFILE_STARMAP, 0x3f, 0);
    ui_data.gfx.starmap.reloc_bu_cancel = lbxfile_item_get(LBXFILE_STARMAP, 0x40, 0);
    ui_data.gfx.starmap.reloc_bu_accept = lbxfile_item_get(LBXFILE_STARMAP, 0x41, 0);
    ui_data.gfx.starmap.tran_bar = lbxfile_item_get(LBXFILE_STARMAP, 0x42, 0);
    for (int i = 0; i < 6; ++i) {
        ui_data.gfx.starmap.smalship[i] = lbxfile_item_get(LBXFILE_STARMAP, 0x43 + i, 0);
        ui_data.gfx.starmap.smaltran[i] = lbxfile_item_get(LBXFILE_STARMAP, 0x49 + i, 0);
        ui_data.gfx.starmap.tinyship[i] = lbxfile_item_get(LBXFILE_STARMAP, 0x4f + i, 0);
        ui_data.gfx.starmap.tinytran[i] = lbxfile_item_get(LBXFILE_STARMAP, 0x55 + i, 0);
    }
    ui_data.gfx.starmap.move_shi = lbxfile_item_get(LBXFILE_STARMAP, 0x5b, 0);
    ui_data.gfx.starmap.move_but_p = lbxfile_item_get(LBXFILE_STARMAP, 0x5c, 0);
    ui_data.gfx.starmap.move_but_m = lbxfile_item_get(LBXFILE_STARMAP, 0x5d, 0);
    ui_data.gfx.starmap.move_but_a = lbxfile_item_get(LBXFILE_STARMAP, 0x5e, 0);
    ui_data.gfx.starmap.move_but_n = lbxfile_item_get(LBXFILE_STARMAP, 0x5f, 0);
    ui_data.gfx.starmap.shipbord = lbxfile_item_get(LBXFILE_STARMAP, 0x60, 0);
    ui_data.gfx.starmap.movextra = lbxfile_item_get(LBXFILE_STARMAP, 0x61, 0);
    ui_data.gfx.starmap.movextr2 = lbxfile_item_get(LBXFILE_STARMAP, 0x62, 0);
    ui_data.gfx.starmap.movextr3 = lbxfile_item_get(LBXFILE_STARMAP, 0x63, 0);
    ui_data.gfx.starmap.scanner = lbxfile_item_get(LBXFILE_STARMAP, 0x64, 0);
    ui_data.gfx.starmap.tranship = lbxfile_item_get(LBXFILE_STARMAP, 0x65, 0);
    ui_data.gfx.starmap.tranbord = lbxfile_item_get(LBXFILE_STARMAP, 0x66, 0);
    ui_data.gfx.starmap.tranxtra = lbxfile_item_get(LBXFILE_STARMAP, 0x67, 0);
    ui_data.gfx.starmap.dismiss = lbxfile_item_get(LBXFILE_STARMAP, 0x68, 0);
    ui_data.gfx.starmap.fleetbut_view = lbxfile_item_get(LBXFILE_STARMAP, 0x69, 0);
    ui_data.gfx.starmap.fleetbut_scrap = lbxfile_item_get(LBXFILE_STARMAP, 0x6a, 0);
    ui_data.gfx.starmap.fleetbut_ok = lbxfile_item_get(LBXFILE_STARMAP, 0x6b, 0);
    ui_data.gfx.starmap.fleetbut_down = lbxfile_item_get(LBXFILE_STARMAP, 0x6c, 0);
    ui_data.gfx.starmap.fleetbut_up = lbxfile_item_get(LBXFILE_STARMAP, 0x6d, 0);
    ui_data.gfx.starmap.viewship = lbxfile_item_get(LBXFILE_STARMAP, 0x6e, 0);
    ui_data.gfx.starmap.viewshp2 = lbxfile_item_get(LBXFILE_STARMAP, 0x6f, 0);
    ui_data.gfx.starmap.viewshbt = lbxfile_item_get(LBXFILE_STARMAP, 0x70, 0);
    ui_data.gfx.starmap.scrap = lbxfile_item_get(LBXFILE_STARMAP, 0x71, 0);
    ui_data.gfx.starmap.scrapbut_no = lbxfile_item_get(LBXFILE_STARMAP, 0x72, 0);
    ui_data.gfx.starmap.scrapbut_yes = lbxfile_item_get(LBXFILE_STARMAP, 0x73, 0);
    ui_data.gfx.starmap.reprtbut_ok = lbxfile_item_get(LBXFILE_STARMAP, 0x75, 0);
    ui_data.gfx.starmap.reprtbut_up = lbxfile_item_get(LBXFILE_STARMAP, 0x76, 0);
    ui_data.gfx.starmap.reprtbut_down = lbxfile_item_get(LBXFILE_STARMAP, 0x77, 0);
    ui_data.gfx.starmap.gr_arrow_u = lbxfile_item_get(LBXFILE_STARMAP, 0x78, 0);
    ui_data.gfx.starmap.gr_arrow_d = lbxfile_item_get(LBXFILE_STARMAP, 0x79, 0);
    ui_data.gfx.starmap.slanbord = lbxfile_item_get(LBXFILE_STARMAP, 0x74, 0);
    for (int i = 0; i < 0x23; ++i) {
        ui_data.gfx.planets.planet[i] = lbxfile_item_get(LBXFILE_PLANETS, i, 0);
    }
    for (int i = 0; i < 10; ++i) {
        ui_data.gfx.planets.race[i] = lbxfile_item_get(LBXFILE_PLANETS, 0x23 + i, 0);
    }
    ui_data.gfx.planets.smonster = lbxfile_item_get(LBXFILE_PLANETS, 0x2d, 0);
    ui_data.gfx.planets.tmonster = lbxfile_item_get(LBXFILE_PLANETS, 0x2e, 0);

    for (int i = 0; i < 0x48; ++i) {
        uint8_t *t;
        if (i < 0x46) {
            t = lbxfile_item_get(LBXFILE_SHIPS2, i, 0);
        } else if (i == 0x46) {
            t = lbxfile_item_get(LBXFILE_PLANETS, i - 0x17, 0);
        } else /*(i == 0x47)*/ {
            t = lbxfile_item_get(LBXFILE_SCREENS, 7, 0);
        }
        ui_data.gfx.ships[i] = t;
        ui_data.gfx.ships[0x48 + i] = lbxfile_item_get(LBXFILE_SHIPS, i, 0);
    }
    for (int i = 0; i < 3; ++i) {
        ui_data.gfx.ships[0x48 * 2 + i] = lbxfile_item_get(LBXFILE_SHIPS2, 0x48 + i, 0);
    }

    ui_data.gfx.screens.tech_but_up = lbxfile_item_get(LBXFILE_SCREENS, 1, 0);
    ui_data.gfx.screens.tech_but_down = lbxfile_item_get(LBXFILE_SCREENS, 2, 0);
    ui_data.gfx.screens.tech_but_ok = lbxfile_item_get(LBXFILE_SCREENS, 3, 0);
    ui_data.gfx.screens.litebulb_off = lbxfile_item_get(LBXFILE_SCREENS, 4, 0);
    ui_data.gfx.screens.litebulb_on = lbxfile_item_get(LBXFILE_SCREENS, 5, 0);
    ui_data.gfx.screens.techback = lbxfile_item_get(LBXFILE_SCREENS, 6, 0);
    ui_data.gfx.screens.race_pnt = lbxfile_item_get(LBXFILE_SCREENS, 8, 0);
    ui_data.gfx.screens.races_bu.sabotage = lbxfile_item_get(LBXFILE_SCREENS, 0x9, 0);
    ui_data.gfx.screens.races_bu.espionage = lbxfile_item_get(LBXFILE_SCREENS, 0xa, 0);
    ui_data.gfx.screens.races_bu.hiding = lbxfile_item_get(LBXFILE_SCREENS, 0xb, 0);
    ui_data.gfx.screens.races_bu.status = lbxfile_item_get(LBXFILE_SCREENS, 0xc, 0);
    ui_data.gfx.screens.races_bu.report = lbxfile_item_get(LBXFILE_SCREENS, 0xd, 0);
    ui_data.gfx.screens.races_bu.audience = lbxfile_item_get(LBXFILE_SCREENS, 0xe, 0);
    ui_data.gfx.screens.races_bu.ok = lbxfile_item_get(LBXFILE_SCREENS, 0xf, 0);

    init_lbx_design();
    init_lbx_space();
    ui_data.gfx.starmap.stargate2 = lbxfile_item_get(LBXFILE_V11, 5, 0);

    for (int i = 0; i < 0x1c; ++i) {
        ui_data.gfx.colonies.d[i] = lbxfile_item_get(LBXFILE_COLONIES, i, 0);
    }
    ui_data.gfx.colonies.current = NULL;
    init_lbx_news();

    ui_data.gfx.vgafileh = lib_malloc(UI_SCREEN_W * UI_SCREEN_H);

    gfx_aux_setup_wh(&ui_data.aux.screen, UI_SCREEN_W, UI_SCREEN_H);
    gfx_aux_setup_wh(&ui_data.aux.ship_p1, 34, 26);
    gfx_aux_setup_wh(&ui_data.aux.ship_overlay, 34, 26);
    gfx_aux_setup_wh(&ui_data.aux.btemp, 38, 30);

    /* load musics 5..8, TODO? */
    ui_data.gfx.initialized = true;
}

/* -------------------------------------------------------------------------- */

int ui_early_init(void)
{
    return 0;
}

int ui_init(void)
{
    memset(&ui_data, 0, sizeof(ui_data));
    return 0;
}

int ui_late_init(void)
{
    ui_enable_unofficial_1_3a();
    if (ui_fix_bugs) {
        ui_enable_fix_bugs();
    }
    if (ui_fix_qol) {
        ui_enable_fix_bugs();
        ui_enable_fix_qol();
    }
    mouse_set_limits(UI_SCREEN_W, UI_SCREEN_H);
    if (0
     || lbxfont_init()
     || hw_video_init(UI_SCREEN_W, UI_SCREEN_H)
     || lbxpal_init()
    ) {
        return 1;
    }
    if (opt_audio_enabled) {
        uint32_t t0, t1;
        t0 = os_get_time_us();
        log_message("Preparing sounds, this may take a while...\n");
        for (int i = NUM_SOUNDS - 1; i >= 0; --i) {
            uint32_t len;
            ui_data.sfx[i] = lbxfile_item_get(LBXFILE_SOUNDFX, i, &len);
            hw_audio_sfx_init(i, ui_data.sfx[i], len);
        }
        t1 = os_get_time_us();
        log_message("Preparing sounds took %i ms\n", (t1 - t0) / 1000);
    }
    ui_data.music_i = -1;
    init_gfx();
    uiobj_table_clear();
    return 0;
}

void ui_shutdown(void)
{
    hw_audio_music_stop();
    hw_audio_sfx_stop();
    if (ui_data.mus) {
        hw_audio_music_release(0);
        lbxfile_item_release(LBXFILE_MUSIC, ui_data.mus);
    }
    for (int i = 0; i < NUM_SOUNDS; ++i) {
        if (ui_data.sfx[i]) {
            hw_audio_sfx_release(i);
            lbxfile_item_release(LBXFILE_SOUNDFX, ui_data.sfx[i]);
        }
    }
    if (ui_data.gfx.initialized) {
        gfx_aux_free(&ui_data.starmap.star_aux);
        gfx_aux_free(&ui_data.aux.screen);
        gfx_aux_free(&ui_data.aux.ship_p1);
        gfx_aux_free(&ui_data.aux.ship_overlay);
        gfx_aux_free(&ui_data.aux.btemp);
        memset(&ui_data.aux, 0, sizeof(ui_data.aux));
        lbxfile_item_release_file(LBXFILE_STARMAP);
        lbxfile_item_release_file(LBXFILE_NEBULA);
        lbxfile_item_release_file(LBXFILE_SHIPS);
        lbxfile_item_release_file(LBXFILE_SHIPS2);
        lbxfile_item_release_file(LBXFILE_SCREENS);
        lbxfile_item_release_file(LBXFILE_RESEARCH);
        lbxfile_item_release_file(LBXFILE_DIPLOMAT);
        lbxfile_item_release_file(LBXFILE_DESIGN);
        lbxfile_item_release_file(LBXFILE_SPACE);
        lbxfile_item_release_file(LBXFILE_MISSILE);
        lbxfile_item_release_file(LBXFILE_V11);
        lbxfile_item_release_file(LBXFILE_COLONIES);
        lbxfile_item_release_file(LBXFILE_NEWSCAST);
        lib_free(ui_data.gfx.vgafileh);
        memset(&ui_data.gfx, 0, sizeof(ui_data.gfx));
    }
    lbxfont_shutdown();
}

char *ui_get_strbuf(void)
{
    return ui_data.strbuf;
}

uint8_t *ui_gfx_get_ship(int look)
{
    return ui_data.gfx.ships[look];
}

uint8_t *ui_gfx_get_planet(int look)
{
    return ui_data.gfx.planets.planet[look];
}

uint8_t *ui_gfx_get_rock(int look)
{
    return ui_data.gfx.space.asteroid[look];
}
