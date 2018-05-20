#include "config.h"

#include <stdio.h>
#include <string.h>

#include "ui.h"
#include "game_new.h"
#include "game_str.h"
#include "types.h"
#include "uiinput.h"
#include "uiload.h"

/* -------------------------------------------------------------------------- */

static bool ui_new_game(struct game_new_options_s *newopts)
{
    int v;
    char *p;

    struct input_list_s ng_galaxy_in[] = {
        { 0, "1", NULL, game_str_tbl_gsize[0] },
        { 1, "2", NULL, game_str_tbl_gsize[1] },
        { 2, "3", NULL, game_str_tbl_gsize[2] },
        { 3, "4", NULL, game_str_tbl_gsize[3] },
        { -1, "Q", NULL, "(quit)" },
        { 0, NULL, NULL, NULL },
    };

    struct input_list_s ng_diffic_in[] = {
        { 0, "1", NULL, game_str_tbl_diffic[0] },
        { 1, "2", NULL, game_str_tbl_diffic[1] },
        { 2, "3", NULL, game_str_tbl_diffic[2] },
        { 3, "4", NULL, game_str_tbl_diffic[3] },
        { 4, "5", NULL, game_str_tbl_diffic[4] },
        { -1, "Q", NULL, "(quit)" },
        { 0, NULL, NULL, NULL },
    };

    struct input_list_s ng_oppon_in[] = {
        { 2, "1", NULL, game_str_tbl_oppon[0] },
        { 3, "2", NULL, game_str_tbl_oppon[1] },
        { 4, "3", NULL, game_str_tbl_oppon[2] },
        { 5, "4", NULL, game_str_tbl_oppon[3] },
        { 6, "5", NULL, game_str_tbl_oppon[4] },
        { -1, "Q", NULL, "(quit)" },
        { 0, NULL, NULL, NULL },
    };

    struct input_list_s ng_race_in[] = {
        { RACE_HUMAN, "1", NULL, game_str_tbl_race[RACE_HUMAN] },
        { RACE_MRRSHAN, "2", NULL, game_str_tbl_race[RACE_MRRSHAN] },
        { RACE_SILICOID, "3", NULL, game_str_tbl_race[RACE_SILICOID] },
        { RACE_SAKKRA, "4", NULL, game_str_tbl_race[RACE_SAKKRA] },
        { RACE_PSILON, "5", NULL, game_str_tbl_race[RACE_PSILON] },
        { RACE_ALKARI, "6", NULL, game_str_tbl_race[RACE_ALKARI] },
        { RACE_KLACKON, "7", NULL, game_str_tbl_race[RACE_KLACKON] },
        { RACE_BULRATHI, "8", NULL, game_str_tbl_race[RACE_BULRATHI] },
        { RACE_MEKLAR, "9", NULL, game_str_tbl_race[RACE_MEKLAR] },
        { RACE_DARLOK, "0", NULL, game_str_tbl_race[RACE_DARLOK] },
        { RACE_RANDOM, "R", NULL, game_str_tbl_race[RACE_RANDOM] },
        { -1, "Q", NULL, "(quit)" },
        { 0, NULL, NULL, NULL },
    };

    struct input_list_s ng_banner_in[] = {
        { BANNER_BLUE, "1", NULL, game_str_tbl_banner[BANNER_BLUE] },
        { BANNER_GREEN, "2", NULL, game_str_tbl_banner[BANNER_GREEN] },
        { BANNER_PURPLE, "3", NULL, game_str_tbl_banner[BANNER_PURPLE] },
        { BANNER_RED, "4", NULL, game_str_tbl_banner[BANNER_RED] },
        { BANNER_WHITE, "5", NULL, game_str_tbl_banner[BANNER_WHITE] },
        { BANNER_YELLOW, "6", NULL, game_str_tbl_banner[BANNER_YELLOW] },
        { BANNER_RANDOM, "7", NULL, game_str_tbl_banner[BANNER_RANDOM] },
        { -1, "Q", NULL, "(quit)" },
        { 0, NULL, NULL, NULL },
    };

    if ((v = ui_input_list("Galaxy size", "> ", ng_galaxy_in)) < 0) {
        return false;
    }
    newopts->galaxy_size = v;
    if ((v = ui_input_list("Difficulty", "> ", ng_diffic_in)) < 0) {
        return false;
    }
    newopts->difficulty = v;
    if ((v = ui_input_list("Opponents", "> ", ng_oppon_in)) < 0) {
        return false;
    }
    newopts->players = v;
    if ((v = ui_input_list("Race", "> ", ng_race_in)) < 0) {
        return false;
    }
    newopts->pdata[PLAYER_0].race = v;
    if ((v = ui_input_list("Banner", "> ", ng_banner_in)) < 0) {
        return false;
    }
    newopts->pdata[PLAYER_0].banner = v;
    fputs("Your name\n", stdout);
    p = ui_input_line_len_trim("> ", EMPEROR_NAME_LEN);
    strcpy(newopts->pdata[PLAYER_0].playername, p);
    fputs("Home world\n", stdout);
    p = ui_input_line_len_trim("> ", PLANET_NAME_LEN);
    strcpy(newopts->pdata[PLAYER_0].homename, p);
    return true;
}

/* -------------------------------------------------------------------------- */

main_menu_action_t ui_main_menu(struct game_new_options_s *newopts, int *load_game_i_ptr)
{
    struct input_list_s main_menu_in[] = {
        { MAIN_MENU_ACT_CONTINUE_GAME, "C", NULL, game_str_mm_continue },
        { MAIN_MENU_ACT_LOAD_GAME, "L", NULL, game_str_mm_load },
        { MAIN_MENU_ACT_NEW_GAME, "N", NULL, game_str_mm_new },
        { MAIN_MENU_ACT_QUIT_GAME, "Q", NULL, game_str_mm_quit },
        { MAIN_MENU_ACT_TUTOR, NULL, "tutor", NULL },
        { 0, NULL, NULL, NULL },
    };

    main_menu_action_t ret = MAIN_MENU_ACT_QUIT_GAME;
    bool flag_done = false;
    while (!flag_done) {
        ret = ui_input_list("Main menu", "> ", main_menu_in);
        switch (ret) {
            case MAIN_MENU_ACT_NEW_GAME:
                flag_done = ui_new_game(newopts);
                break;
            case MAIN_MENU_ACT_LOAD_GAME:
                {
                    int i;
                    i = ui_load_game();
                    if (i >= 0) {
                        *load_game_i_ptr = i;
                        flag_done = true;
                    }
                }
                break;
            default:
                flag_done = true;
                break;
        }
    }
    return ret;
}
