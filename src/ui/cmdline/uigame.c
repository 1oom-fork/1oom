#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "game.h"
#include "game_save.h"
#include "game_shipdesign.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "uicheat.h"
#include "uicmds.h"
#include "uidefs.h"
#include "uiempire.h"
#include "uifleet.h"
#include "uihelp.h"
#include "uiinput.h"
#include "uiload.h"
#include "uiplanet.h"
#include "uiraces.h"
#include "uiswitch.h"
#include "uitech.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

static int cmd_next(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    return 0;
}

static const struct input_cmd_s * const cmdsptr_turn[];

static const struct input_cmd_s cmds_turn[] = {
    { "?", NULL, "Help", 0, 0, 0, ui_cmd_help, (void *)cmdsptr_turn },
    { "n", NULL, "Next turn", 0, 0, 0, cmd_next, 0 },
    { "l", "[PLANET|*]", "Look", 0, 1, 0, ui_cmd_planet_look, 0 },
    { "g", "PLANET", "Go to planet", 1, 1, 0, ui_cmd_planet_go, 0 },
    { "s", "SLIDER VALUE", "Set planet slider\nSLIDER is s, d, i, e or t\nVALUE can be +N or -N for relative adjustment", 2, 2, 0, ui_cmd_planet_slider, 0 },
    { "sl", "SLIDER", "Toggle planet slider lock", 1, 1, 0, ui_cmd_planet_slider_lock, 0 },
    { "b", "[SHIP]", "Select ship to build", 0, 1, 0, ui_cmd_planet_build, 0 },
    { "gov", NULL, "Toggle planetary governor", 0, 0, 0, ui_cmd_planet_govern, 0 },
    { "reloc", "[PLANET]", "Relocate built ships to", 0, 1, 0, ui_cmd_planet_reloc, 0 },
    { "trans", "[PLANET NUM]", "Transport troops to", 0, 2, 0, ui_cmd_planet_trans, 0 },
    { "res", "[BC]", "Transfer reserves", 0, 1, 0, ui_cmd_planet_reserve, 0 },
    { "fs", "PLANET [NUM]*", "Send fleet to\nAll ships are sent if no NUM given", 1, 7, 0, ui_cmd_fleet_send, 0 },
    { "t", "[FIELD]", "View technology\nFIELD is c, o, f, p, r or w\nShows sliders if no field given", 0, 1, 0, ui_cmd_tech_look, 0 },
    { "ts", "FIELD VALUE", "Set tech slider to value\nVALUE can be +N or -N for relative adjustment", 2, 2, 0, ui_cmd_tech_slider, 0 },
    { "tsl", "FIELD", "Toggle tech slider lock", 1, 1, 0, ui_cmd_tech_slider_lock, 0 },
    { "t=", NULL, "Equalize tech sliders", 0, 0, 0, ui_cmd_tech_equals, 0 },
    { "emp", NULL, "Empire overview", 0, 0, 0, ui_cmd_empire_look, 0 },
    { "tax", "VALUE", "Set tax (tenths of percent)\nVALUE can be +N or -N for relative adjustment", 1, 1, 0, ui_cmd_empire_tax, 0 },
    { "sec", "VALUE", "Set security spending (tenths of percent)\nVALUE can be +N or -N for relative adjustment", 1, 1, 0, ui_cmd_empire_security, 0 },
    { "aud", NULL, "Audience", 0, 0, 0, ui_cmd_audience, 0 },
    { "galaxy", NULL, NULL, 0, 0, 0, ui_cmd_cheat_galaxy, 0 },
    { "events", NULL, NULL, 0, 0, 0, ui_cmd_cheat_events, 0 },
    { "moola", NULL, NULL, 0, 0, 0, ui_cmd_cheat_moola, 0 },
    { NULL, NULL, NULL, 0, 0, 0, NULL, 0 }
};

static const struct input_cmd_s * const cmdsptr_turn[] = {
    cmds_turn,
    ui_cmds_opts,
    NULL
};

/* -------------------------------------------------------------------------- */

ui_turn_action_t ui_game_turn(struct game_s *g, int *load_game_i_ptr, int pi)
{
    ui_switch_1_opts(g, pi);
    while (1) {
        char *input;
        char prompt[80], buf_planet_name[PLANET_NAME_LEN];
        sprintf(prompt, "%s | %i | %s > ", g->emperor_names[pi], g->year + YEAR_BASE, ui_planet_str(g, pi, g->planet_focus_i[pi], buf_planet_name));
        input = ui_input_line(prompt);
        if ((ui_input_tokenize(input, cmdsptr_turn) == 0) && (ui_data.input.num > 0)) {
            if (ui_data.input.tok[0].type == INPUT_TOKEN_COMMAND) {
                const struct input_cmd_s *cmd;
                int v;
                cmd = ui_data.input.tok[0].data.cmd;
                v = cmd->handle(g, pi, &ui_data.input.tok[1], ui_data.input.num - 1, cmd->var);
                if (v >= 0) {
                    if (cmd->handle == cmd_next) {
                        return UI_TURN_ACT_NEXT_TURN;
                    }
                    if (cmd->handle == ui_cmd_load) {
                        *load_game_i_ptr = v;
                        return UI_TURN_ACT_LOAD_GAME;
                    }
                    if (cmd->handle == ui_cmd_quit) {
                        return UI_TURN_ACT_QUIT_GAME;
                    }
                }
            }
        }
    }
}

void ui_game_start(struct game_s *g)
{
    BOOLVEC_CLEAR(ui_data.players_viewing, PLAYER_NUM);
    fputs("Welcome! Type ? to get the list of commands.\n", stdout);
}

void ui_game_end(struct game_s *g)
{
}
