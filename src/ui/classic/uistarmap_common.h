#ifndef INC_1OOM_UISTARMAP_COMMON_H
#define INC_1OOM_UISTARMAP_COMMON_H

#include "game.h"
#include "types.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uiobj.h"

#if 0
/* original */
#define STARMAP_DELAY 3
#define STARMAP_ANIM_DELAY 1
#define STARMAP_SCROLLSTEP 10
#else
#define STARMAP_DELAY 1
#define STARMAP_ANIM_DELAY 3
#define STARMAP_SCROLLSTEP  ui_sm_scroll_speed
#endif

#define STARMAP_LIM_INIT()  const int slx0 = (6 * ui_scale) / starmap_scale, sly0 = (6 * ui_scale) / starmap_scale, slx1 = (222 * ui_scale - 1) / starmap_scale, sly1 = (178 * ui_scale - 1) / starmap_scale
#define STARMAP_TEXT_LIMITS 6 * ui_scale, 6 * ui_scale, 222 * ui_scale - 1, 178 * ui_scale - 1
#define STARMAP_LIMITS  slx0, sly0, slx1, sly1

struct shipnon0_s {
    shipcount_t ships[NUM_SHIPDESIGNS];
    uint8_t type[NUM_SHIPDESIGNS];
    uint8_t num;    /* number of ship types on orbit with nonzero amount */
    bool have_reserve_fuel;
};

typedef enum { NO_MOVE, GOT_HYPERCOMM, ON_PLANET } can_move_t;

struct starmap_data_s {
    struct game_s *g; /* FIXME non-const only for ui_starmap_draw_cb1 */
    player_id_t api;
    int16_t scroll_x;
    int16_t scroll_y;
    uint8_t scroll_z;
    int bottom_highlight;
    int anim_delay;
    uint8_t from_i;
    uint8_t hover_i;
    uint8_t ruler_to_i;
    bool ruler_from_fleet;
    int gov_highlight;
    bool controllable;
    bool order_mode;
    bool hide_focus;
    bool disable_tags;
    bool (*update_cb) (struct starmap_data_s *, bool first_time);
    bool (*valid_target_cb) (const struct starmap_data_s *, int planet_i);
    void (*on_accept_cb) (struct starmap_data_s *);
    void (*on_pos_focus_cb) (struct starmap_data_s *);
    int16_t oi_gameopts;
    int16_t oi_design;
    int16_t oi_fleet;
    int16_t oi_map;
    int16_t oi_races;
    int16_t oi_planets;
    int16_t oi_tech;
    int16_t oi_next_turn;
    int16_t oi_accept;
    int16_t oi_scroll;
    int16_t oi_search;
    int16_t oi_ctrl_r;
    int16_t oi_alt_r;
    int16_t oi_alt_f;
    int16_t oi_alt_m;
    int16_t oi_alt_o;
    int16_t oi_alt_c;
    int16_t oi_finished;
    int16_t oi_tbl_stars[PLANETS_MAX];
    int16_t oi_ctrl_left;
    int16_t oi_ctrl_l2;
    int16_t oi_ctrl_right;
    int16_t oi_ctrl_r2;
    int16_t oi_ctrl_ul;
    int16_t oi_ctrl_ur;
    int16_t oi_ctrl_up;
    int16_t oi_ctrl_u2;
    int16_t oi_ctrl_dl;
    int16_t oi_ctrl_down;
    int16_t oi_ctrl_d2;
    int16_t oi_ctrl_dr;
    int16_t oi_f2;
    int16_t oi_f3;
    int16_t oi_f8;
    int16_t oi_f9;
    int16_t oi_tag_set[PLANET_TAG_NUM];
    int16_t oi_tag_get[PLANET_TAG_NUM];
    int16_t oi_tbl_enroute[FLEET_ENROUTE_MAX];
    int16_t oi_tbl_transport[TRANSPORT_MAX];
    int16_t oi_tbl_pl_stars[PLAYER_NUM][PLANETS_MAX];
    union {
        struct {
            int16_t oi_ship;
            int16_t oi_reloc;
            int16_t oi_trans;
            int16_t oi_tbl_slider_lock[PLANET_SLIDER_NUM];
            int16_t oi_tbl_slider_minus[PLANET_SLIDER_NUM];
            int16_t oi_tbl_slider_plus[PLANET_SLIDER_NUM];
            int16_t oi_gov_ship;
            int16_t oi_gov_reserve;
            int16_t oi_gov_tech;
            int16_t oi_gov_bases;
            int16_t oi_gov_wheel_bases;
            int16_t oi_gov_boost;
        } sm;   /* starmap_do */
        struct {
        } rl;   /* reloc */
        struct {
            int16_t num;
            bool other;
            bool blink;
        } tr;   /* trans */
        struct {
            can_move_t can_move;
            struct draw_stars_s ds;
            int frame_ship;
            int frame_scanner;
            int scanner_delay;
        } ts;   /* transport */
        struct {
            shipcount_t ships[NUM_SHIPDESIGNS];
            uint8_t shiptypenon0numsel; /* number of ship types selected with nonzero amount */
            struct shipnon0_s sn0;
            int cursor_over;
        } oo;   /* orbit_own */
        struct {
            shipcount_t ships[NUM_SHIPDESIGNS];
            struct shipnon0_s sn0;
            player_id_t player;
            int frame_scanner;
            int scanner_delay;
            int yoff;
        } oe;   /* orbit_en */
        struct {
            struct shipnon0_s sn0;
            uint8_t pon;
            can_move_t can_move;
            struct draw_stars_s ds;
            int frame_ship;
            int frame_scanner;
            int scanner_delay;
        } en;   /* enroute */
    };
};

#define STARMAP_UIOBJ_CLEAR_COMMON() \
    do { \
        d.oi_gameopts = UIOBJI_INVALID; \
        d.oi_design = UIOBJI_INVALID; \
        d.oi_fleet = UIOBJI_INVALID; \
        d.oi_map = UIOBJI_INVALID; \
        d.oi_races = UIOBJI_INVALID; \
        d.oi_planets = UIOBJI_INVALID; \
        d.oi_tech = UIOBJI_INVALID; \
        d.oi_next_turn = UIOBJI_INVALID; \
        d.oi_accept = UIOBJI_INVALID; \
        d.oi_scroll = UIOBJI_INVALID; \
        d.oi_search = UIOBJI_INVALID; \
        d.oi_ctrl_r = UIOBJI_INVALID; \
        d.oi_alt_r = UIOBJI_INVALID; \
        d.oi_alt_f = UIOBJI_INVALID; \
        d.oi_alt_m = UIOBJI_INVALID; \
        d.oi_alt_o = UIOBJI_INVALID; \
        d.oi_alt_c = UIOBJI_INVALID; \
        d.oi_finished = UIOBJI_INVALID; \
        d.oi_f2 = UIOBJI_INVALID; \
        d.oi_f3 = UIOBJI_INVALID; \
        d.oi_f8 = UIOBJI_INVALID; \
        d.oi_f9 = UIOBJI_INVALID; \
        for (int i = 0; i < g->galaxy_stars; ++i) { \
            d.oi_tbl_stars[i] = UIOBJI_INVALID; \
        } \
        UIOBJI_SET_TBL_INVALID(d.oi_tbl_enroute); \
        UIOBJI_SET_TBL_INVALID(d.oi_tbl_transport); \
        for (int i = 0; i < g->galaxy_stars; ++i) { \
            for (int j = 0; j < g->players; ++j) { \
                d.oi_tbl_pl_stars[j][i] = UIOBJI_INVALID; \
            } \
        } \
        ui_starmap_clear_oi_ctrl(&d); \
    } while (0)

#define STARMAP_UIOBJ_CLEAR_FX() \
    do { \
        oi_f4 = UIOBJI_INVALID; \
        oi_f5 = UIOBJI_INVALID; \
        oi_f6 = UIOBJI_INVALID; \
        oi_f7 = UIOBJI_INVALID; \
        oi_f10 = UIOBJI_INVALID; \
    } while (0)

extern const uint8_t colortbl_textbox[5];
extern const uint8_t colortbl_line_red[5];
extern const uint8_t colortbl_line_reloc[5];
extern const uint8_t colortbl_line_green[5];

extern void ui_starmap_clear_oi_ctrl(struct starmap_data_s *d);
extern void ui_starmap_draw_basic(struct starmap_data_s *d);
extern void ui_starmap_draw_starmap(struct starmap_data_s *d);
extern void ui_starmap_draw_button_text(struct starmap_data_s *d, bool highlight);
extern void ui_starmap_sn0_setup(struct shipnon0_s *sn0, int sd_num, const shipcount_t *ships);
extern void ui_starmap_update_reserve_fuel(struct game_s *g, struct shipnon0_s *sn0, const shipcount_t *ships, player_id_t pi);
extern void ui_starmap_draw_planetinfo(const struct game_s *g, player_id_t api, int planet_i);
extern void ui_starmap_draw_planetinfo_2(const struct game_s *g, int p1, int p2, int planet_i);
extern int ui_starmap_newship_next(const struct game_s *g, player_id_t pi, int i);
extern int ui_starmap_newship_prev(const struct game_s *g, player_id_t pi, int i);
extern int ui_starmap_enemy_incoming(const struct game_s *g, player_id_t pi, int i, bool next);
extern void ui_starmap_compute_scale(const struct game_s *g);
extern void ui_starmap_select_bottom_highlight(struct starmap_data_s *d, int16_t oi);
extern void ui_starmap_set_ruler(struct starmap_data_s *d, int16_t oi);
extern bool ui_starmap_handle_oi_finished(struct game_s *g, struct starmap_data_s *d, bool *flag_done, int16_t oi1, int16_t oi2);

extern bool ui_starmap_common_init(struct game_s *g, struct starmap_data_s *d, player_id_t active_player);
extern bool ui_starmap_common_late_init(struct starmap_data_s *d, void (*draw_cb) (void *), bool controllable);
extern bool ui_starmap_common_handle_oi(struct game_s *g, struct starmap_data_s *d, bool *flag_done, int16_t oi1, int16_t oi2);
extern void ui_starmap_common_fill_oi(struct starmap_data_s *d);

#endif
