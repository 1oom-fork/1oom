#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "cfg.h"
#include "game.h"
#include "gameapi.h"
#include "game_ai.h"
#include "game_aux.h"
#include "game_misc.h"
#include "game_new.h"
#include "game_nump.h"
#include "game_end.h"
#include "game_save.h"
#include "game_strp.h"
#include "game_turn.h"
#include "game_tech.h"
#include "log.h"
#include "options.h"
#include "rnd.h"
#include "ui.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

static bool game_opt_skip_intro_always = false;
static bool game_opt_skip_intro = false;
static bool game_opt_new_game = false;
static bool game_opt_continue = false;
static int game_opt_load_game = 0;
static const char *game_opt_load_fname = 0;
static bool game_opt_undo_enabled = true;
static bool game_opt_year_save_enabled = false;
static bool game_opt_next_turn = false;
static bool game_opt_save_quit = false;

static struct game_end_s game_opt_end = { GAME_END_NONE, 0, 0, 0, 0 };
static struct game_new_options_s game_opt_new = GAME_NEW_OPTS_DEFAULT;

static int game_opt_new_value = 200;

static struct game_s game;
static struct game_aux_s game_aux;


/* -------------------------------------------------------------------------- */

static void game_start(struct game_s *g)
{
    if (g->seed == 0) {
        g->seed = rnd_get_new_seed();
        log_message("Game: seed was 0, got new seed 0x%0x\n", g->seed);
    }
    if (g->ai_id >= GAME_AI_NUM) {
        log_warning("Game: AI ID was %i >= %i, setting to %i (%s)\n", g->ai_id, GAME_AI_NUM, GAME_AI_CLASSIC, game_ais[GAME_AI_CLASSIC]->name);
        g->ai_id = GAME_AI_CLASSIC;
    }
    game_ai = game_ais[g->ai_id];
    game_update_production(g);
    game_update_tech_util(g);
    game_update_within_range(g);
    game_update_visibility(g);
    game_update_have_reserve_fuel(g);
}

static void game_stop(struct game_s *g)
{
    g->gaux->flag_cheat_galaxy = false;
    g->gaux->flag_cheat_events = false;
}

static void game_set_opts_from_value(struct game_new_options_s *go, int v)
{
    int v2;
    v2 = v % 10;
    v = v / 10;
    go->difficulty = v2;
    v2 = v % 10;
    v = v / 10;
    go->galaxy_size = v2;
    go->players = v;
}

static int game_get_opts_value(const struct game_s *g)
{
    return g->difficulty + g->galaxy_size * 10 + g->players * 100;
}

/* -------------------------------------------------------------------------- */

static int game_opt_do_new_seed(char **argv, void *var)
{
    uint32_t vo, vr, vb, vs, va;
    char buf[512];
    char *stropt = NULL;
    char *strrace = NULL;
    char *strbanner = NULL;
    char *strseed = NULL;
    char *strhuman = NULL;
    strncpy(buf, argv[1], 511);
    buf[511] = 0;
    {
        char *p = buf;
        stropt = (*p != ':') ? p : NULL;
        p = strchr(p, ':');
        if (p) {
            *p++ = 0;
            strrace = (*p != ':') ? p : NULL;
            p = strchr(p, ':');
            if (p) {
                *p++ = 0;
                strbanner = (*p != ':') ? p : NULL;
                p = strchr(p, ':');
                if (p) {
                    *p++ = 0;
                    strseed = (*p != ':') ? p : NULL;
                    p = strchr(p, ':');
                    if (p) {
                        *p++ = 0;
                        strhuman = (*p != ':') ? p : NULL;
                    }
                }
            }
        }
    }
    {
        uint32_t v = 0, v2;
        if (stropt) {
            if (!util_parse_number(stropt, &v)) {
                log_error("invalid value '%s'\n", stropt);
                return -1;
            }
        } else {
            v = game_opt_new_value;
        }
        vo = v;
        v2 = v % 10;
        v = v / 10;
        if (v2 > DIFFICULTY_NUM) {
            log_error("invalid difficulty num %i\n", v2);
            return -1;
        }
        game_opt_new.difficulty = v2;
        v2 = v % 10;
        v = v / 10;
        if (v2 > GALAXY_SIZE_HUGE) {
            log_error("invalid galaxy size num %i\n", v2);
            return -1;
        }
        game_opt_new.galaxy_size = v2;
        v2 = v % 10;
        if ((v2 < 2) || (v2 > PLAYER_NUM)) {
            log_error("invalid players num %i\n", v2);
            return -1;
        }
        game_opt_new.players = v2;
    }
    {
        uint32_t v = 0, v2;
        if (strrace) {
            if (!util_parse_number(strrace, &v)) {
                log_error("invalid value '%s'\n", strrace);
                return -1;
            }
        } else {
            v = 0;
        }
        vr = v;
        for (int i = 0; i < PLAYER_NUM; ++i) {
            v2 = v & 0xf;
            v = v >> 4;
            if (v2 > RACE_NUM) {
                log_error("invalid race num %i\n", v2);
                return -1;
            }
            v2 = v2 ? (v2 - 1) : RACE_RANDOM;
            game_opt_new.pdata[i].race = v2;
        }
    }
    {
        uint32_t v = 0, v2;
        if (strbanner) {
            if (!util_parse_number(strbanner, &v)) {
                log_error("invalid value '%s'\n", strbanner);
                return -1;
            }
        } else {
            v = 0;
        }
        vb = v;
        for (int i = 0; i < PLAYER_NUM; ++i) {
            v2 = v % 10;
            v = v / 10;
            if (v2 > BANNER_NUM) {
                log_error("invalid banner num %i\n", v2);
                return -1;
            }
            v2 = v2 ? (v2 - 1) : BANNER_RANDOM;
            game_opt_new.pdata[i].banner = v2;
        }
    }
    {
        uint32_t v = 0;
        if (strseed) {
            if (!util_parse_number(strseed, &v)) {
                log_error("invalid value '%s'\n", strseed);
                return -1;
            }
        } else {
            v = 0;
        }
        vs = v;
        game_opt_new.galaxy_seed = v;
    }
    {
        uint32_t v = 0, v2;
        if (strhuman) {
            if (!util_parse_number(strhuman, &v)) {
                log_error("invalid value '%s'\n", strhuman);
                return -1;
            }
        } else {
            v = 1;
        }
        va = v;
        for (int i = 0; i < PLAYER_NUM; ++i) {
            v2 = v % 10;
            v = v / 10;
            game_opt_new.pdata[i].is_ai = !v2;
        }
    }
    game_opt_skip_intro = true;
    game_opt_load_game = 0;
    game_opt_load_fname = 0;
    game_opt_continue = false;
    game_opt_new_game = true;
    log_message("Game: -new %u:0x%x:%u:0x%x:%u\n", vo, vr, vb, vs, va);
    return 0;
}

static int game_opt_set_new_name(char **argv, void *var)
{
    uint32_t v = 0;
    char *buf;
    if (!util_parse_number(argv[1], &v)) {
        log_error("invalid value '%s'\n", argv[1]);
        return -1;
    } else if ((v < 1) || (v > PLAYER_NUM)) {
        log_error("invalid player num %i\n", v);
        return -1;
    }
    buf = game_opt_new.pdata[v - 1].playername;
    strncpy(buf, argv[2], EMPEROR_NAME_LEN);
    buf[EMPEROR_NAME_LEN - 1] = '\0';
    log_message("Game: player %i name '%s'\n", v, buf);
    return 0;
}

static int game_opt_set_new_home(char **argv, void *var)
{
    uint32_t v = 0;
    char *buf;
    if (!util_parse_number(argv[1], &v)) {
        log_error("invalid value '%s'\n", argv[1]);
        return -1;
    } else if ((v < 1) || (v > PLAYER_NUM)) {
        log_error("invalid player num %i\n", v);
        return -1;
    }
    buf = game_opt_new.pdata[v - 1].homename;
    strncpy(buf, argv[2], PLANET_NAME_LEN);
    buf[PLANET_NAME_LEN - 1] = '\0';
    log_message("Game: player %i home '%s'\n", v, buf);
    return 0;
}

static int game_opt_set_new_ai(char **argv, void *var)
{
    uint32_t v = 0;
    if (!util_parse_number(argv[1], &v)) {
        log_error("invalid value '%s'\n", argv[1]);
        return -1;
    } else if (v > GAME_AI_NUM) {
        log_error("invalid AI num %i\n", v);
        return -1;
    }
    game_opt_new.ai_id = v;
    log_message("Game: AI type %i '%s'\n", v, game_ais[v]->name);
    return 0;
}


static int game_opt_do_load(char **argv, void *var)
{
    uint32_t v = 0;
    if (1
      && util_parse_number(argv[1], &v)
      && (((v >= 1) && (v <= NUM_ALL_SAVES)) || ((v >= 2300) && (v <= 9999)))
    ) {
        game_opt_load_game = v;
        game_opt_load_fname = 0;
        log_message("Game: load game %i\n", game_opt_load_game);
    } else {
        game_opt_load_game = 0;
        game_opt_load_fname = argv[1];
        log_message("Game: load game '%s'\n", game_opt_load_fname);
    }
    game_opt_skip_intro = true;
    game_opt_continue = false;
    game_opt_new_game = false;
    return 0;
}

static int game_opt_do_continue(char **argv, void *var)
{
    game_opt_load_game = 0;
    game_opt_load_fname = 0;
    game_opt_skip_intro = true;
    game_opt_continue = true;
    game_opt_new_game = false;
    log_message("Game: continue game\n");
    return 0;
}

static int dump_strings(char **argv, void *var)
{
    game_str_dump();
    return -1;
}

static int dump_numbers(char **argv, void *var)
{
    game_num_dump();
    return -1;
}

/* -------------------------------------------------------------------------- */

const char *idstr_main = "game";

bool main_use_lbx = true;
bool main_use_cfg = true;

void (*main_usage)(void) = 0;

const struct cmdline_options_s main_cmdline_options_early[] = {
    { "-dumpstr", 0,
      dump_strings, NULL,
      NULL, "Dump strings in PBXIN format" },
    { "-dumpnum", 0,
      dump_numbers, NULL,
      NULL, "Dump numbers in PBXIN format" },
    { 0, 0, 0, 0, 0, 0 }
};

const struct cmdline_options_s main_cmdline_options[] = {
    { "-new", 1,
      game_opt_do_new_seed, 0,
      "GAMESEED", "Start new game using given game seed\n"
                  "GAMESEED is OPT[:RACES[:BANNERS[:GSEED[:HUMANS]]]]\n"
                  "OPT is PLAYERS*100+GALAXYSIZE*10+DIFFICULTY\n  2..6, 0..3 = small..huge, 0..4 = simple..impossible\n  default same as last new game\n"
                  "RACES is PLAYERnRACE*(0x10^n), n=0..5\n  0 = random, 1..0xA = human..darlok\n  default 0 (all random)\n"
                  "BANNERS is PLAYERnBANNER*(10^n), n=0..5\n  0 = random, 1..6 = blue..yellow\n  default 0 (all random)\n"
                  "GSEED is a 32 bit galaxy seed or 0 for random\n  default 0\n"
                  "HUMANS is PLAYERnISHUMAN*(10^n), n=0..5\n  default 1 (player 1 is human, others AI)"
    },
    { "-ngn", 2,
      game_opt_set_new_name, 0,
      "PLAYER NAME", "Set new game emperor name for player 1..6" },
    { "-ngh", 2,
      game_opt_set_new_home, 0,
      "PLAYER NAME", "Set new game home world name for player 1..6" },
    { "-nga", 1,
      game_opt_set_new_ai, 0,
      "AITYPE", "Set new game AI type (0..1)" },
    { "-load", 1,
      game_opt_do_load, 0,
      "SAVE", "Load game (1..8, 2300.. or filename)\n1..6 are regular save slots\n7 is continue game\n8 is undo\n2300 and over are yearly saves" },
    { "-continue", 0,
      game_opt_do_continue, 0,
      NULL, "Continue game" },
    { "-undo", 0,
      options_enable_bool_var, (void *)&game_opt_undo_enabled,
      NULL, "Enable undo saves" },
    { "-noundo", 0,
      options_disable_bool_var, (void *)&game_opt_undo_enabled,
      NULL, "Disable undo saves" },
    { "-yearsave", 0,
      options_enable_bool_var, (void *)&game_opt_year_save_enabled,
      NULL, "Enable yearly saves" },
    { "-noyearsave", 0,
      options_disable_bool_var, (void *)&game_opt_year_save_enabled,
      NULL, "Disable yearly saves" },
    { "-skipintro", 0,
      options_enable_bool_var, (void *)&game_opt_skip_intro_always,
      NULL, "Skip intro" },
    { "-noskipintro", 0,
      options_disable_bool_var, (void *)&game_opt_skip_intro_always,
      NULL, "Do not skip intro" },
    { "-nextturn", 0,
      options_enable_bool_var, (void *)&game_opt_next_turn,
      NULL, "Go directly to next turn (for reproducing bugs)" },
    { "-savequit", 0,
      options_enable_bool_var, (void *)&game_opt_save_quit,
      NULL, "Save and quit (for debugging)" },
    { 0, 0, 0, 0, 0, 0 }
};

/* -------------------------------------------------------------------------- */

static bool game_cfg_check_new_game_opts(void *val)
{
    int v2, v = (int)(intptr_t)val;
    v2 = v % 10;
    v = v / 10;
    if (v2 > DIFFICULTY_NUM) {
        log_error("invalid difficulty num %i\n", v2);
        return false;
    }
    v2 = v % 10;
    v = v / 10;
    if (v2 > GALAXY_SIZE_HUGE) {
        log_error("invalid galaxy size num %i\n", v2);
        return false;
    }
    v2 = v % 10;
    if ((v2 < 2) || (v2 > PLAYER_NUM)) {
        log_error("invalid players num %i\n", v2);
        return false;
    }
    return true;
}

const struct cfg_items_s game_cfg_items[] = {
    CFG_ITEM_BOOL("undo", &game_opt_undo_enabled),
    CFG_ITEM_BOOL("yearsave", &game_opt_year_save_enabled),
    CFG_ITEM_BOOL("skipintro", &game_opt_skip_intro_always),
    CFG_ITEM_COMMENT("PLAYERS*100+GALAXYSIZE*10+DIFFICULTY"),
    CFG_ITEM_COMMENT(" 2..6, 0..3 = small..huge, 0..4 = simple..impossible"),
    CFG_ITEM_INT("new_game_opts", &game_opt_new_value, game_cfg_check_new_game_opts),
    CFG_ITEM_END
};

/* -------------------------------------------------------------------------- */

int main_handle_option(const char *argv)
{
    if (game_opt_end.type == GAME_END_NONE) {
        if ((argv[1] == '\0') && (argv[0] >= '0') && (argv[0] <= '3')) {
            switch (argv[0]) {
                case '0':
                    game_opt_end.type = GAME_END_WON_GOOD;
                    break;
                case '1':
                    game_opt_end.type = GAME_END_LOST_FUNERAL;
                    break;
                case '2':
                    game_opt_end.type = GAME_END_LOST_EXILE;
                    break;
                case '3':
                    game_opt_end.type = GAME_END_WON_TYRANT;
                    break;
            }
            game_opt_load_game = 0;
            game_opt_load_fname = 0;
            game_opt_new_game = false;
            game_opt_skip_intro = true;
            game_opt_continue = false;
            return 0;
        } else if (strcmp(argv, "YOMAMA") == 0) {
            log_message("Game: skip intro for YOMAMA\n");
            game_opt_skip_intro = true;
            return 0;
        } else if (strcmp(argv, "s") == 0) {
            log_message("Game: direct continue\n");
            game_opt_load_game = 0;
            game_opt_load_fname = 0;
            game_opt_new_game = false;
            game_opt_skip_intro = true;
            game_opt_continue = true;
            return 0;
        }
    } else {
        if (game_opt_end.varnum == 0) {
            switch (game_opt_end.type) {
                case GAME_END_LOST_EXILE:
                    game_opt_end.name = argv;
                    break;
                default:
                    game_opt_end.race = atoi(argv);
                    break;
            }
        } else if (game_opt_end.varnum == 1) {
            switch (game_opt_end.type) {
                case GAME_END_WON_GOOD:
                case GAME_END_WON_TYRANT:
                    game_opt_end.name = argv;
                    log_message("Game: ending %s %i '%s'\n", (game_opt_end.type == GAME_END_WON_GOOD) ? "good" : "tyrant", game_opt_end.race, game_opt_end.name);
                    break;
                case GAME_END_LOST_FUNERAL:
                    game_opt_end.banner_dead = atoi(argv);
                    log_message("Game: ending funeral %i %i\n", game_opt_end.race, game_opt_end.banner_dead);
                    break;
                case GAME_END_LOST_EXILE:
                    log_message("Game: ending exile '%s'\n", game_opt_end.name);
                    break;
                default:
                    break;
            }
        } else {
            return -1;
        }
        ++game_opt_end.varnum;
        return 0;
    }
    return -1;
}

int main_do(void)
{
    struct game_end_s game_end = game_opt_end;
    if (ui_late_init()) {
        return 1;
    }
    game_aux_init(&game_aux, &game);
    game_save_check_saves(game_aux.savenamebuf, game_aux.savenamebuflen);
    if ((game_opt_end.type != GAME_END_NONE) && (game_opt_end.varnum == 2)) {
        goto do_ending;
    }
    if (!(game_opt_skip_intro || game_opt_skip_intro_always)) {
        ui_play_intro();
    }
    while (1) {
        struct game_new_options_s game_new_opts = GAME_NEW_OPTS_DEFAULT;
        main_menu_action_t main_menu_action;
        int load_game_i = 0;

        if (game_opt_new_game) {
            game_opt_new_game = false;
            game_new_opts = game_opt_new;
            goto main_menu_new_game;
        } else if (game_opt_load_fname) {
            if (game_save_do_load_fname(game_opt_load_fname, 0, &game)) {
                log_fatal_and_die("Game: could not load save '%s'\n", game_opt_load_fname);
            }
            game_opt_load_fname = 0;
            goto main_menu_start_game;
        } else if (game_opt_load_game) {
            load_game_i = game_opt_load_game;
            if (load_game_i < 2300) {
                --load_game_i;
            }
            game_opt_load_game = 0;
            if ((load_game_i >= 2300) || game_save_tbl_have_save[load_game_i]) {
                goto main_menu_load_game;
            } else {
                log_warning("Game: direct load game %i failed due to missing savegame\n", load_game_i + 1);
                /* try again, now with game_opt_load_game set to 0 */
                continue;
            }
        } else if (game_opt_continue) {
            game_opt_continue = false;
            if (game_save_tbl_have_save[GAME_SAVE_I_CONTINUE]) {
                goto main_menu_continue_game;
            } else {
                log_warning("Game: direct continue failed due to missing savegame\n");
                /* try again, now with game_opt_continue set to false */
                continue;
            }
        } else {
            game_set_opts_from_value(&game_new_opts, game_opt_new_value);
            main_menu_action = ui_main_menu(&game_new_opts, &load_game_i);
        }
        switch (main_menu_action) {
            case MAIN_MENU_ACT_NEW_GAME:
                main_menu_new_game:
                game_new(&game, &game_aux, &game_new_opts);
                game_opt_new_value = game_get_opts_value(&game);
                break;
            case MAIN_MENU_ACT_TUTOR:
                game_new_tutor(&game, &game_aux);
                break;
            case MAIN_MENU_ACT_LOAD_GAME:
                main_menu_load_game:
                if (0
                  || ((load_game_i < NUM_ALL_SAVES) && game_save_do_load_i(load_game_i, &game))
                  || ((load_game_i >= 2300) && game_save_do_load_year(load_game_i, 0, &game))
                ) {
                    log_fatal_and_die("Game: could not load save %i\n", load_game_i);
                }
                break;
            case MAIN_MENU_ACT_CONTINUE_GAME:
                main_menu_continue_game:
                if (game_save_do_load_i(GAME_SAVE_I_CONTINUE, &game)) {
                    log_fatal_and_die("Game: could not start or continue from save 7\n");
                }
                break;
            case MAIN_MENU_ACT_QUIT_GAME:
                log_message("Game: quit (main)\n");
                goto done;
        }
        main_menu_start_game:
        game_aux_start(&game_aux, &game);
        game_start(&game);
        ui_game_start(&game);
        game_end.type = GAME_END_NONE;
        while ((game_end.type == GAME_END_NONE) || (game_end.type == GAME_END_FINAL_WAR)) {
            for (; ((game_end.type == GAME_END_NONE) || (game_end.type == GAME_END_FINAL_WAR)) && (game.active_player < game.players); ++game.active_player) {
                if (IS_AI(&game, game.active_player) || (!IS_ALIVE(&game, game.active_player))) {
                    continue;
                }
                if (game_opt_next_turn) {
                    game_opt_next_turn = false;
                    break;
                }
                if (game_opt_save_quit) {
                    goto turn_act_quit;
                }
                switch (ui_game_turn(&game, &load_game_i, game.active_player)) {
                    case UI_TURN_ACT_LOAD_GAME:
                        main_menu_action = MAIN_MENU_ACT_LOAD_GAME;
                        ui_game_end(&game);
                        goto main_menu_load_game;
                    case UI_TURN_ACT_QUIT_GAME:
                        turn_act_quit:
                        if (game_save_do_save_i(GAME_SAVE_I_CONTINUE, "Continue", &game)) {
                            log_error("Game: could create continue save\n");
                        }
                        game_end.type = GAME_END_QUIT;
                        break;
                    case UI_TURN_ACT_NEXT_TURN:
                        if (game_opt_undo_enabled && game_save_do_save_i(GAME_SAVE_I_UNDO, "Undo", &game)) {
                            log_error("Game: could create undo save\n");
                        }
                        if (game_opt_year_save_enabled && game_save_do_save_year(NULL, &game)) {
                            log_error("Game: could create year save\n");
                        }
                        break;
                }
            }
            if (game_end.type != GAME_END_QUIT) {
                game_end = game_turn_process(&game);
            }
            game.active_player = PLAYER_0;
        }
        ui_game_end(&game);
        do_ending:
        switch (game_end.type) {
            case GAME_END_QUIT:
                log_message("Game: quit (ingame)\n");
                if (game_opt_save_quit) {
                    game_opt_save_quit = false;
                    goto done;
                }
                break;
            case GAME_END_NONE:
            case GAME_END_FINAL_WAR:
                break;
            case GAME_END_WON_GOOD:
                ui_play_ending_good(game_end.race, game_end.name);
                break;
            case GAME_END_WON_TYRANT:
                ui_play_ending_tyrant(game_end.race, game_end.name);
                break;
            case GAME_END_LOST_FUNERAL:
                ui_play_ending_funeral(game_end.race, game_end.banner_dead);
                break;
            case GAME_END_LOST_EXILE:
                ui_play_ending_exile(game_end.name);
                break;
        }
        game_end.type = GAME_END_NONE;
        game_stop(&game);
    }

done:
    return 0;
}

void main_do_shutdown(void)
{
    /* TODO save game if in progress */
    game_aux_shutdown(&game_aux);
    game_str_shutdown();
}
