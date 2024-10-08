#include "config.h"

#include <string.h>

#include "game_strp.h"
#include "gameapi.h"
#include "game_str.h"
#include "game_types.h"
#include "lib.h"
#include "log.h"

/* -------------------------------------------------------------------------- */

static int patch_num = 0;
static char **patch_tbl = NULL;

/* -------------------------------------------------------------------------- */

#define DEFSTRITEMTBL(name) { #name, game_str_##name, sizeof(game_str_##name) / sizeof(game_str_##name[0]) }
#define DEFSTRITEM(name)    { #name, &game_str_##name, 1 }
#define DEFSTREND           { NULL, NULL, 0 }

static const struct strtbl_s {
    const char *strid;
    const char **ptr;
    int size;
} game_str_id_tbl[] = {
    DEFSTRITEM(mm_continue),
    DEFSTRITEM(mm_load),
    DEFSTRITEM(mm_new),
    DEFSTRITEM(mm_quit),
    DEFSTRITEMTBL(tbl_race),
    DEFSTRITEMTBL(tbl_races),
    DEFSTRITEMTBL(tbl_banner),
    DEFSTRITEMTBL(tbl_gsize),
    DEFSTRITEMTBL(tbl_diffic),
    DEFSTRITEMTBL(tbl_oppon),
    DEFSTRITEMTBL(tbl_traits),
    DEFSTRITEMTBL(tbl_trait1),
    DEFSTRITEMTBL(tbl_trait2),
    DEFSTRITEM(ng_choose_race),
    DEFSTRITEM(ng_choose_banner),
    DEFSTRITEM(ng_your_name),
    DEFSTRITEM(ng_home_name),
    DEFSTRITEM(ng_ai),
    DEFSTRITEM(ng_computer),
    DEFSTRITEM(ng_player),
    DEFSTRITEM(ng_cancel),
    DEFSTRITEM(ng_ok),
    DEFSTRITEM(ng_allai),
    DEFSTRITEMTBL(tbl_planet_names),
    DEFSTRITEMTBL(tbl_home_names),
    DEFSTRITEM(rndempname),
    DEFSTRITEM(planet_name_orion),
    DEFSTRITEMTBL(tbl_ship_names),
    DEFSTRITEMTBL(tbl_stship_names),
    DEFSTRITEMTBL(tbl_monsh_names),
    DEFSTRITEMTBL(tbl_mon_names),
    DEFSTRITEM(ai_colonyship),
    DEFSTRITEM(st_none),
    DEFSTRITEM(st_none2),
    DEFSTRITEMTBL(tbl_st_weap),
    DEFSTRITEMTBL(tbl_st_weapx),
    DEFSTRITEMTBL(tbl_st_comp),
    DEFSTRITEMTBL(tbl_st_engine),
    DEFSTRITEMTBL(tbl_st_armor),
    DEFSTRITEMTBL(tbl_st_shield),
    DEFSTRITEMTBL(tbl_st_jammer),
    DEFSTRITEMTBL(tbl_st_specsh),
    DEFSTRITEMTBL(tbl_st_special),
    DEFSTRITEMTBL(tbl_st_specialx),
    DEFSTRITEMTBL(tbl_st_hull),
    DEFSTRITEM(sm_crystal),
    DEFSTRITEM(sm_amoeba),
    DEFSTRITEM(sm_game),
    DEFSTRITEM(sm_design),
    DEFSTRITEM(sm_fleet),
    DEFSTRITEM(sm_map),
    DEFSTRITEM(sm_races),
    DEFSTRITEM(sm_planets),
    DEFSTRITEM(sm_tech),
    DEFSTRITEM(sm_next_turn),
    DEFSTRITEMTBL(tbl_sm_stinfo),
    DEFSTRITEM(sm_range),
    DEFSTRITEM(sm_parsec),
    DEFSTRITEM(sm_parsecs),
    DEFSTRITEM(sm_parsecs2),
    DEFSTRITEM(sm_colony),
    DEFSTRITEM(sm_lastrep),
    DEFSTRITEM(sm_stargate),
    DEFSTRITEM(sm_prodnone),
    DEFSTRITEM(sm_prod_y),
    DEFSTRITEM(sm_defupg),
    DEFSTRITEM(sm_defshld),
    DEFSTRITEM(sm_refit),
    DEFSTRITEM(sm_indmax),
    DEFSTRITEM(sm_indres),
    DEFSTRITEM(sm_ecowaste),
    DEFSTRITEM(sm_ecoclean),
    DEFSTRITEM(sm_ecoatmos),
    DEFSTRITEM(sm_ecotform),
    DEFSTRITEM(sm_ecosoil),
    DEFSTRITEM(sm_ecogaia),
    DEFSTRITEM(sm_ecopop),
    DEFSTRITEM(sm_unexplored),
    DEFSTRITEM(sm_nohabit),
    DEFSTRITEM(sm__planets),
    DEFSTRITEMTBL(tbl_sm_pltype),
    DEFSTRITEM(sm_plague),
    DEFSTRITEM(sm_nova),
    DEFSTRITEM(sm_comet),
    DEFSTRITEM(sm_pirates),
    DEFSTRITEM(sm_rebellion),
    DEFSTRITEM(sm_unrest),
    DEFSTRITEM(sm_accident),
    DEFSTRITEMTBL(tbl_sm_pgrowth),
    DEFSTRITEMTBL(tbl_sm_pspecial),
    DEFSTRITEM(sm_pop),
    DEFSTRITEM(sm_max),
    DEFSTRITEM(sm_hasreached),
    DEFSTRITEM(sm_indmaxof),
    DEFSTRITEM(sm_factories),
    DEFSTRITEM(sm_extrares),
    DEFSTRITEM(sm_popmaxof),
    DEFSTRITEM(sm_colonists),
    DEFSTRITEM(sm_hasterraf),
    DEFSTRITEMTBL(tbl_sm_terraf),
    DEFSTRITEM(sm_envwith),
    DEFSTRITEMTBL(tbl_sm_envmore),
    DEFSTRITEM(sm_stdgrow),
    DEFSTRITEM(sm_hasfsgate),
    DEFSTRITEM(sm_hasfshield),
    DEFSTRITEM(sm_planshield),
    DEFSTRITEM(sm_planratio),
    DEFSTRITEM(sm_fleetdep),
    DEFSTRITEM(sm_destoor),
    DEFSTRITEM(sm_destoor2),
    DEFSTRITEM(sm_parsfromcc),
    DEFSTRITEM(sm_eta),
    DEFSTRITEM(sm_turn),
    DEFSTRITEM(sm_turns),
    DEFSTRITEM(sm_chdest),
    DEFSTRITEM(sm_outsr),
    DEFSTRITEM(sm_sreloc),
    DEFSTRITEM(sm_sreloc2),
    DEFSTRITEM(sm_delay),
    DEFSTRITEM(sm_seltr),
    DEFSTRITEM(sm_notrange),
    DEFSTRITEM(sm_notrange1),
    DEFSTRITEM(sm_notrange2),
    DEFSTRITEM(sm_notrange3),
    DEFSTRITEM(sm_trfirste),
    DEFSTRITEM(sm_trcontr1),
    DEFSTRITEM(sm_trcontr2),
    DEFSTRITEM(sm_trfirstc),
    DEFSTRITEM(sm_trwarna),
    DEFSTRITEM(sm_trwarnm1),
    DEFSTRITEM(sm_trwarnm2),
    DEFSTRITEM(sm_trchnum1),
    DEFSTRITEM(sm_trchnum2),
    DEFSTRITEM(sm_trans1),
    DEFSTRITEM(sm_transs),
    DEFSTRITEM(sm_tdest),
    DEFSTRITEM(sm_bomb1),
    DEFSTRITEM(sm_bomb2),
    DEFSTRITEM(sm_trinb1),
    DEFSTRITEM(sm_trinb1s),
    DEFSTRITEM(sm_trinb2),
    DEFSTRITEM(sm_obomb1),
    DEFSTRITEM(sm_obomb2),
    DEFSTRITEM(sm_cdest1),
    DEFSTRITEM(sm_cdest2),
    DEFSTRITEM(sm_ineff1),
    DEFSTRITEM(sm_ineff2),
    DEFSTRITEM(sm_bkill1),
    DEFSTRITEM(sm_bkill2),
    DEFSTRITEM(sm_bfact1),
    DEFSTRITEM(sm_bfact1s),
    DEFSTRITEM(sm_bfact2),
    DEFSTRITEM(sm_traad1),
    DEFSTRITEM(sm_traad2),
    DEFSTRITEM(sm_trbdb1),
    DEFSTRITEM(sm_trbdb2),
    DEFSTRITEM(sm_inorbit),
    DEFSTRITEMTBL(tbl_roman), /* FIXME do we really need this to be modifiable? */
    DEFSTRITEM(no_events),
    DEFSTRITEM(bc),
    DEFSTRITEM(y),
    DEFSTRITEM(year),
    DEFSTRITEM(player),
    DEFSTRITEM(pl_reserve),
    DEFSTRITEM(pl_plague),
    DEFSTRITEM(pl_nova),
    DEFSTRITEM(pl_comet),
    DEFSTRITEM(pl_pirates),
    DEFSTRITEM(pl_rebellion),
    DEFSTRITEM(pl_unrest),
    DEFSTRITEM(pl_accident),
    DEFSTRITEM(pl_spending),
    DEFSTRITEM(pl_tincome),
    DEFSTRITEM(pl_transof),
    DEFSTRITEM(pl_resto),
    DEFSTRITEM(sd_cancel),
    DEFSTRITEM(sd_build),
    DEFSTRITEM(sd_clear),
    DEFSTRITEM(sd_comp),
    DEFSTRITEM(sd_shield),
    DEFSTRITEM(sd_ecm),
    DEFSTRITEM(sd_armor),
    DEFSTRITEM(sd_engine),
    DEFSTRITEM(sd_man),
    DEFSTRITEMTBL(tbl_sd_spec),
    DEFSTRITEMTBL(tbl_sd_weap),
    DEFSTRITEM(sd_count),
    DEFSTRITEM(sd_sweap),
    DEFSTRITEM(sd_damage),
    DEFSTRITEM(sd_rng),
    DEFSTRITEM(sd_notes),
    DEFSTRITEM(sd_hp),
    DEFSTRITEM(sd_warp),
    DEFSTRITEM(sd_def),
    DEFSTRITEM(sd_cspeed),
    DEFSTRITEM(sd_absorbs),
    DEFSTRITEM(sd_hit),
    DEFSTRITEM(sd_hits),
    DEFSTRITEM(sd_misdef),
    DEFSTRITEM(sd_att),
    DEFSTRITEM(sd_comptype),
    DEFSTRITEM(sd_cost),
    DEFSTRITEM(sd_size),
    DEFSTRITEM(sd_power),
    DEFSTRITEM(sd_space),
    DEFSTRITEM(sd_comps),
    DEFSTRITEM(sd_shieldtype),
    DEFSTRITEM(sd_shields),
    DEFSTRITEM(sd_ecmtype),
    DEFSTRITEM(sd_ecm2),
    DEFSTRITEM(sd_armortype),
    DEFSTRITEM(sd_armor2),
    DEFSTRITEM(sd_engtype),
    DEFSTRITEM(sd_numengs),
    DEFSTRITEM(sd_engs),
    DEFSTRITEM(sd_man1),
    DEFSTRITEM(sd_man2),
    DEFSTRITEM(sd_class),
    DEFSTRITEM(sd_speed),
    DEFSTRITEM(sd_max),
    DEFSTRITEM(sd_weapname),
    DEFSTRITEM(sd_descr),
    DEFSTRITEM(sd_dmg),
    DEFSTRITEM(sd_weaps),
    DEFSTRITEM(sd_specname),
    DEFSTRITEM(sd_specs),
    DEFSTRITEM(sp_only6),
    DEFSTRITEM(sp_wantscrap),
    DEFSTRITEM(sp_before),
    DEFSTRITEM(sp_cost),
    DEFSTRITEM(fl_station),
    DEFSTRITEM(fl_inorbit),
    DEFSTRITEM(fl_moving),
    DEFSTRITEM(fl_unknown),
    DEFSTRITEM(fl_system),
    DEFSTRITEM(gm_tchar),
    DEFSTRITEMTBL(tbl_gm_spec),
    DEFSTRITEM(gm_unable),
    DEFSTRITEM(gm_prod),
    DEFSTRITEM(gm_tech),
    DEFSTRITEM(gm_1_3),
    DEFSTRITEM(gm_1_2),
    DEFSTRITEM(gm_2x),
    DEFSTRITEM(gm_3x),
    DEFSTRITEM(gm_4x),
    DEFSTRITEM(gm_prodb1),
    DEFSTRITEM(gm_prodb2),
    DEFSTRITEM(gm_prodb3),
    DEFSTRITEM(gm_gmap),
    DEFSTRITEM(gm_mapkey),
    DEFSTRITEM(bs_line1),
    DEFSTRITEM(bs_line2),
    DEFSTRITEM(bs_bases),
    DEFSTRITEMTBL(tbl_te_field),
    DEFSTRITEM(te_adv),
    DEFSTRITEM(te_tech),
    DEFSTRITEM(te_techno),
    DEFSTRITEM(te_techno2),
    DEFSTRITEM(te_genimp),
    DEFSTRITEM(te_nmis),
    DEFSTRITEM(te_nbomb),
    DEFSTRITEM(te_scrange),
    DEFSTRITEM(te_rctrl),
    DEFSTRITEM(te_col),
    DEFSTRITEM(te_fwaste),
    DEFSTRITEM(te_gcombat),
    DEFSTRITEM(te_tform),
    DEFSTRITEM(te_wasteel),
    DEFSTRITEM(te_shrange),
    DEFSTRITEM(te_max),
    DEFSTRITEM(te_rp),
    DEFSTRITEM(nt_achieve),
    DEFSTRITEM(nt_break),
    DEFSTRITEM(nt_infil),
    DEFSTRITEM(nt_ruins),
    DEFSTRITEM(nt_orion),
    DEFSTRITEM(nt_scouts),
    DEFSTRITEM(nt_choose),
    DEFSTRITEM(nt_reveal),
    DEFSTRITEM(nt_secrets),
    DEFSTRITEM(nt_frame),
    DEFSTRITEM(nt_victim),
    DEFSTRITEM(nt_doyou),
    DEFSTRITEM(nt_inc),
    DEFSTRITEM(nt_redueco),
    DEFSTRITEM(nt_ind),
    DEFSTRITEM(nt_ecoall),
    DEFSTRITEM(nt_terra),
    DEFSTRITEM(nt_def),
    DEFSTRITEM(nt_ecostd),
    DEFSTRITEM(nt_ecohost),
    DEFSTRITEMTBL(tbl_nt_adj),
    DEFSTRITEM(ra_nocont),
    DEFSTRITEM(ra_notpres),
    DEFSTRITEM(ra_secline1),
    DEFSTRITEM(ra_secline2),
    DEFSTRITEM(ra_alloc),
    DEFSTRITEM(ra_planres),
    DEFSTRITEM(ra_diplo),
    DEFSTRITEM(ra_gone),
    DEFSTRITEM(ra_nospies),
    DEFSTRITEM(ra_spy),
    DEFSTRITEM(ra_spies),
    DEFSTRITEMTBL(tbl_ra_treaty),
    DEFSTRITEM(ra_trade),
    DEFSTRITEM(ra_notrade),
    DEFSTRITEMTBL(tbl_ra_relat),
    DEFSTRITEM(ra_stats),
    DEFSTRITEM(re_reportis),
    DEFSTRITEM(re_current),
    DEFSTRITEM(re_yearsold),
    DEFSTRITEM(re_alliance),
    DEFSTRITEM(re_wars),
    DEFSTRITEM(re_environ),
    DEFSTRITEM(sc_caught),
    DEFSTRITEM(bp_scombat),
    DEFSTRITEM(bp_attack),
    DEFSTRITEM(bp_attacks),
    DEFSTRITEM(bp_won),
    DEFSTRITEM(bt_auto_move),
    DEFSTRITEM(bt_pop),
    DEFSTRITEM(bt_ind),
    DEFSTRITEM(bt_bases),
    DEFSTRITEM(bt_subint),
    DEFSTRITEM(bt_launch),
    DEFSTRITEM(bt_coldest),
    DEFSTRITEM(es_youresp1),
    DEFSTRITEM(es_youresp2),
    DEFSTRITEM(es_youresp3),
    DEFSTRITEM(es_thesp1),
    DEFSTRITEM(es_thesp2),
    DEFSTRITEM(es_unkn),
    DEFSTRITEM(sb_choose),
    DEFSTRITEM(sb_lastrep),
    DEFSTRITEM(sb_pop),
    DEFSTRITEM(sb_fact),
    DEFSTRITEM(sb_bases),
    DEFSTRITEM(sb_unkn),
    DEFSTRITEM(sb_your),
    DEFSTRITEM(sb_spies),
    DEFSTRITEM(sb_increv),
    DEFSTRITEM(sb_inc1),
    DEFSTRITEM(sb_inc2),
    DEFSTRITEM(sb_destr),
    DEFSTRITEM(sb_fact2),
    DEFSTRITEM(sb_facts),
    DEFSTRITEM(sb_mbase),
    DEFSTRITEM(sb_mbases),
    DEFSTRITEM(sb_failed),
    DEFSTRITEM(sb_nofact),
    DEFSTRITEM(sb_nobases),
    DEFSTRITEM(sb_noinc),
    DEFSTRITEM(sb_frame),
    DEFSTRITEM(ex_planeta),
    DEFSTRITEM(ex_scanner),
    DEFSTRITEM(ex_scout),
    DEFSTRITEM(ex_explore),
    DEFSTRITEM(ex_starsys),
    DEFSTRITEM(ex_build),
    DEFSTRITEM(ex_colony),
    DEFSTRITEM(ex_popgr),
    DEFSTRITEM(ex_resopnt),
    DEFSTRITEM(ex_fromind),
    DEFSTRITEM(ex_techpnt),
    DEFSTRITEM(ex_fromres),
    DEFSTRITEM(ex_aredbl),
    DEFSTRITEM(ex_arequad),
    DEFSTRITEMTBL(ex_pg1),
    DEFSTRITEMTBL(ex_pg2),
    DEFSTRITEMTBL(ex_pg3),
    DEFSTRITEMTBL(ex_ps1),
    DEFSTRITEMTBL(ex_ps2),
    DEFSTRITEM(la_colony),
    DEFSTRITEM(la_inyear),
    DEFSTRITEM(la_the),
    DEFSTRITEM(la_formnew),
    DEFSTRITEM(gr_carmor),
    DEFSTRITEM(gr_outof),
    DEFSTRITEM(gr_transs),
    DEFSTRITEM(gr_reclaim),
    DEFSTRITEM(gr_penetr),
    DEFSTRITEM(gr_defenss),
    DEFSTRITEM(gr_troops),
    DEFSTRITEM(gr_rebel),
    DEFSTRITEM(gr_gcon),
    DEFSTRITEM(gr_scapt),
    DEFSTRITEM(gr_itroops),
    DEFSTRITEM(gr_succd),
    DEFSTRITEM(gr_fcapt),
    DEFSTRITEM(gr_tsteal),
    DEFSTRITEM(gr_tnew),
    DEFSTRITEM(el_no),
    DEFSTRITEM(el_vote),
    DEFSTRITEM(el_votes),
    DEFSTRITEM(el_total),
    DEFSTRITEM(el_start),
    DEFSTRITEM(el_emperor),
    DEFSTRITEM(el_ofthe),
    DEFSTRITEM(el_and),
    DEFSTRITEM(el_for),
    DEFSTRITEM(el_nomin),
    DEFSTRITEM(el_abs1),
    DEFSTRITEM(el_abs2),
    DEFSTRITEM(el_dots),
    DEFSTRITEM(el_your),
    DEFSTRITEM(el_bull),
    DEFSTRITEM(el_self),
    DEFSTRITEM(el_abs),
    DEFSTRITEM(el_neither),
    DEFSTRITEM(el_accept),
    DEFSTRITEM(el_yes),
    DEFSTRITEM(el_no2),
    DEFSTRITEM(el_sobeit),
    DEFSTRITEM(el_isnow),
    DEFSTRITEM(au_facts),
    DEFSTRITEM(au_bases),
    DEFSTRITEM(au_treaty),
    DEFSTRITEM(au_allian),
    DEFSTRITEM(au_nonagg),
    DEFSTRITEM(au_tradea),
    DEFSTRITEM(au_amreca),
    DEFSTRITEM(au_tech),
    DEFSTRITEM(au_framed),
    DEFSTRITEM(au_bull),
    DEFSTRITEM(au_inxchng),
    DEFSTRITEM(au_whatif1),
    DEFSTRITEM(au_whatif2),
    DEFSTRITEM(au_perrec1),
    DEFSTRITEM(au_ques),
    DEFSTRITEM(au_howmay),
    DEFSTRITEM(au_youprte),
    DEFSTRITEM(au_youprta),
    DEFSTRITEM(au_youract),
    DEFSTRITEM(au_whatech),
    DEFSTRITEM(au_whatrad),
    DEFSTRITEM(au_whatoff),
    DEFSTRITEM(au_perthr1),
    DEFSTRITEM(au_perthr2),
    DEFSTRITEM(au_alsoof1),
    DEFSTRITEM(au_alsoof2),
    DEFSTRITEM(au_whowar),
    DEFSTRITEM(au_whobrk),
    DEFSTRITEM(au_whattr),
    DEFSTRITEM(au_techn),
    DEFSTRITEM(au_nextp),
    DEFSTRITEM(au_back),
    DEFSTRITEMTBL(au_opts_main),
    DEFSTRITEMTBL(au_opts_treaty),
    DEFSTRITEMTBL(au_opts_agree),
    DEFSTRITEMTBL(au_opts_accept),
    DEFSTRITEMTBL(au_opts_threaten),
    DEFSTRITEMTBL(au_optsmp1),
    DEFSTRITEM(tr_cont1),
    DEFSTRITEM(tr_cont2),
    DEFSTRITEM(tr_fuel1),
    DEFSTRITEM(tr_fuel2),
    DEFSTRITEM(sv_envir),
    DEFSTRITEM(sv_stargt),
    DEFSTRITEM(sv_shild1),
    DEFSTRITEM(sv_shild2),
    DEFSTRITEM(sv_psize),
    DEFSTRITEM(sv_fact),
    DEFSTRITEM(sv_waste),
    DEFSTRITEM(sv_pop),
    DEFSTRITEM(sv_growth),
    DEFSTRITEM(sv_techp),
    DEFSTRITEM(sv_resp),
    DEFSTRITEM(sv_1_3x),
    DEFSTRITEM(sv_1_2x),
    DEFSTRITEM(sv_2x),
    DEFSTRITEM(sv_3x),
    DEFSTRITEM(sv_4x),
    DEFSTRITEM(sv_popgr),
    DEFSTRITEMTBL(sv_pg1),
    DEFSTRITEMTBL(sv_pg2),
    DEFSTRITEM(in_loading),
    DEFSTRITEM(wl_won_1),
    DEFSTRITEM(wl_won_2),
    DEFSTRITEM(wl_won_3),
    DEFSTRITEM(wl_3_good_1),
    DEFSTRITEM(wl_3_good_2),
    DEFSTRITEM(wl_3_tyrant_1),
    DEFSTRITEM(wl_3_tyrant_2),
    DEFSTRITEM(wl_3_tyrant_3),
    DEFSTRITEM(wl_3_tyrant_4),
    DEFSTRITEM(wl_exile_1),
    DEFSTRITEM(wl_exile_2),
    DEFSTRITEM(wl_exile_3),
    DEFSTRITEM(wl_exile_4),
    DEFSTRITEM(gnn_end_good),
    DEFSTRITEM(gnn_end_tyrant),
    DEFSTRITEM(gnn_also),
    DEFSTREND
};

/* -------------------------------------------------------------------------- */

static const char **find_match(const char *strid, int i)
{
    const struct strtbl_s *s = &game_str_id_tbl[0];
    while (s->strid) {
        if (strcmp(s->strid, strid) == 0) {
            if (i < s->size) {
                return &s->ptr[i];
            } else {
                log_error("STR: strid '%s' index %i >= size %i\n", strid, i, s->size);
                return NULL;
            }
        }
        ++s;
    }
    log_error("STR: unknown strid '%s'\n", strid);
    return NULL;
}

/* -------------------------------------------------------------------------- */

bool game_str_patch(const char *strid, const char *patchstr, int i)
{
    const char **p = find_match(strid, i);
    if (p) {
        char *n = lib_stralloc(patchstr);
        patch_tbl = lib_realloc(patch_tbl, patch_num + 1);
        patch_tbl[patch_num++] = n;
        *p = n;
        return true;
    } else {
        return false;
    }
}

void game_str_dump(void)
{
    const struct strtbl_s *s = &game_str_id_tbl[0];
    log_message("# dump of all game strings\n");
    while (s->strid) {
        for (int i = 0; (i < s->size); ++i) {
            log_message("3,%s,%i,\"%s\"\n", s->strid, i, s->ptr[i]);
        }
        ++s;
    }
}

void game_str_shutdown(void)
{
    if (patch_tbl) {
        for (int i = 0; i < patch_num; ++i) {
            lib_free(patch_tbl[i]);
            patch_tbl[i] = NULL;
        }
        lib_free(patch_tbl);
        patch_tbl = NULL;
    }
}
