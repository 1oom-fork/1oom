#include "common.h"

const int chunk0_start = 0x75792;
const int chunk0_end = 0x75794;
const uint8_t chunk0_ui_qol_starmap_no_qmark_cursor[] = {
    /*75792*/ 0xeb, 0x07,
};

const int chunk1_0_start = 0x6d8f5;
const int chunk1_0_end = 0x6d8fe;
const uint8_t chunk1_0_game_fix_space_scanners[] = {
    /*6d8f5*/ 0x83, 0x7e, 0xe6, 0x00,
    /*6d8f9*/ 0x7f, 0x03,
    /*6d8fb*/ 0xe9, 0xd8, 0x00,
};

const int chunk1_1_start = 0x6d96e;
const int chunk1_1_end = 0x6d993;
const uint8_t chunk1_1_game_fix_space_scanners[] = {
    /*6d96e*/ 0x8b, 0x46, 0xec,
    /*6d971*/ 0xbb, 0x0a, 0x00,
    /*6d974*/ 0x99,
    /*6d975*/ 0xf7, 0xfb,
    /*6d977*/ 0x0b, 0xd2,
    /*6d979*/ 0x75, 0x0b,
    /*6d97b*/ 0x8b, 0x46, 0xec,
    /*6d97e*/ 0xbb, 0x0a, 0x00,
    /*6d981*/ 0x99,
    /*6d982*/ 0xf7, 0xfb,
    /*6d984*/ 0xeb, 0x0a,
    /*6d986*/ 0x8b, 0x46, 0xec,
    /*6d989*/ 0xbb, 0x0a, 0x00,
    /*6d98c*/ 0x99,
    /*6d98d*/ 0xf7, 0xfb,
    /*6d98f*/ 0x40,
    /*6d990*/ 0x89, 0x46, 0xec,
};

const int chunk1_2_start = 0x6d99b;
const int chunk1_2_end = 0x6d9a1;
const uint8_t chunk1_2_game_fix_space_scanners[] = {
    /*6d99b*/ 0x8b, 0x46, 0xf2,
    /*6d99e*/ 0x89, 0x46, 0xec,
};
const uint8_t subst1_2_game_fix_space_scanners[] = {
    /*6d99b*/ 0x8b, 0x46, 0xec,
    /*6d99e*/ 0x89, 0x46, 0xf2,
};

const int chunk2_0_off = 0x7a6e5;
const int chunk2_1_off = 0x7a6f7;
const int chunk2_2_off = 0x7a70d;
const int chunk2_3_off = 0x7a723;
const int chunk3_0_off = 0x7a73d;
const int chunk3_1_off = 0x7a74f;
const int chunk3_2_off = 0x7a765;
const int chunk3_3_off = 0x7a77b;
const uint8_t chunk2_ui_fix_starmap_background[] = {
    /*7a6e5*/ 0xff, 0x36, 0x0c, 0x77,
};
const uint8_t chunk3_ui_fix_starmap_background[] = {
    /*7a74f*/ 0xff, 0x36, 0x0a, 0x77,
};

const int chunk4_start = 0x7c83e;
const int chunk4_end = 0x7c841;
const uint8_t chunk4_ui_fix_spy_cost[] = {
    /*7c83e*/ 0xf7, 0x6e, 0xd8,
};

const int chunk5_start = 0x64c4e;
const int chunk5_end = 0x64c51;
const uint8_t chunk5_ui_fix_tech_complete_probability[] = {
    /*64c4e*/ 0xba, 0x02, 0x00,
};
const uint8_t subst5_ui_fix_tech_complete_probability[] = {
    /*64c4e*/ 0xba, 0x01, 0x00,
};

const int chunk6_start = 0x79c08;
const int chunk6_end = 0x79c0d;
const uint8_t chunk6_ui_qol_starmap_msg_pos[] = {
    /*79c08*/ 0xc7, 0x46, 0xf6, 0x36, 0x00,
};
const uint8_t subst6_ui_qol_starmap_msg_pos[] = {
    /*79c08*/ 0xc7, 0x46, 0xf6, 0x2d, 0x00,
};

const int chunk7_start = 0x69f72;
const int chunk7_end = 0x69f8a;
const uint8_t chunk7_game_ai_fix_spy_hiding[] = {
    /*0x69f72*/ 0x8b, 0x1e, 0xaa, 0xd4,
    /*0x69f76*/ 0xd1, 0xe3,
    /*0x69f78*/ 0xc7, 0x87, 0x6a, 0x79, 0x00, 0x00,
    /*0x69f7e*/ 0x8b, 0x1e, 0xaa, 0xd4,
    /*0x69f82*/ 0xd1, 0xe3,
    /*0x69f84*/ 0xc7, 0x87, 0x36, 0x7b, 0x00, 0x00,
};
const uint8_t subst7_game_ai_fix_spy_hiding[] = {
    /*0x69f72*/ 0x90, 0x90,
    /*0x69f74*/ 0xa1, 0xaa, 0xd4,
    /*0x69f77*/ 0xba, 0xd4, 0x0d,
    /*0x69f7a*/ 0xf7, 0xea,
    /*0x69f7c*/ 0x8b, 0xd8,
    /*0x69f7e*/ 0xc7, 0x87, 0x6a, 0x79, 0x00, 0x00,
    /*0x69f84*/ 0xc7, 0x87, 0x36, 0x7b, 0x00, 0x00,
};

const int chunk8_start = 0x6cc9;
const int chunk8_end = 0x6ccb;
const uint8_t chunk8_main_allow_direct_launch[] = {
    /*0x6cc9*/ 0x75, 0x08,
};
const uint8_t subst8_main_allow_direct_launch[] = {
    /*0x6cc9*/ 0xeb, 0x12,
};

const int chunk9_1_start = 0x77f31;
const int chunk9_1_end = 0x77f34;
const int chunk9_2_start = 0x78830;
const int chunk9_2_end = 0x78833;
const uint8_t chunk9_ui_qol_extra_key_bindings_space[] = {
    /*0x77f31*/ 0xb8, 0xa2, 0x50,
};
const uint8_t subst9_ui_qol_extra_key_bindings_space[] = {
    /*0x77f31*/ 0xb8, 0xa8, 0x47,
};

const int chunk10_1_start = 0xf126;
const int chunk10_1_end = 0xf127;
const int chunk10_2_start = 0xf14a;
const int chunk10_2_end = 0xf14b;
const uint8_t chunk10_game_fix_sg_finished[] = {
    /*0xf126*/ 0x52,
};
const uint8_t replace10_game_fix_sg_finished[] = {
    /*0xf126*/ 0x50,
};

const int chunk11_1_start = 0xb1e7;
const int chunk11_1_end = 0xb1e9;
const int chunk11_2_start = 0xb1fd;
const int chunk11_2_end = 0xb1ff;
const uint8_t chunk11_game_fix_max_factories[] = {
    /*0xb1e7*/ 0xc4, 0x09,
};
const uint8_t replace11_game_fix_max_factories[] = {
    /*0xb1e7*/ 0x8c, 0x0a,
};

const int chunk12_start = 0x61e39;
const int chunk12_end = 0x61e44;
const uint8_t chunk12_ui_fix_planet_list_pos[] = {
    /*0x61e39*/ 0x8b, 0xc7,
    /*0x61e3b*/ 0x05, 0xfb, 0xff,
    /*0x61e3e*/ 0xa3, 0x0c, 0xd2,
    /*0x61e41*/ 0xa1, 0x10, 0xd2,
};
const uint8_t replace12_ui_fix_planet_list_pos[] = {
    /*0x61e39*/ 0xa1, 0x10,
    /*0x61e3b*/ 0xd2, 0x50, 0x05,
    /*0x61e3e*/ 0xfb, 0xff, 0xa3,
    /*0x61e41*/ 0x0c, 0xd2, 0x58,
};

const int chunk13_1_start = 0x70638;
const int chunk13_1_end = 0x70644;
const uint8_t chunk13_1_ui_qol_no_cancel_via_lmb[] = {
    /*0x70638*/ 0x50,
    /*0x70639*/ 0x33, 0xc0,
    /*0x7063b*/ 0x50,
    /*0x7063c*/ 0x9a, 0xec, 0x3b, 0x68, 0x01,
    /*0x70641*/ 0x83, 0xc4, 0x0c,
};
const uint8_t replace13_1_ui_qol_no_cancel_via_lmb[] = {
    /*0x70638*/ 0xb8, 0xa6, 0x47,
    /*0x7063b*/ 0x50,
    /*0x7063c*/ 0x9a, 0x7a, 0x3e, 0x68, 0x01,
    /*0x70641*/ 0x59,
    /*0x70642*/ 0x90, 0x90,
};
const int chunk13_2_start = 0x70626;
const int chunk13_2_end = 0x70628;
const uint8_t chunk13_2_ui_qol_no_cancel_via_lmb[] = {
    /*0x70626*/ 0xb8, 0xff,
};
const uint8_t replace13_2_ui_qol_no_cancel_via_lmb[] = {
    /*0x70626*/ 0xeb, 0x10,
};
const int chunk13_3_start = 0x705e9;
const int chunk13_3_end = 0x705ea;
const uint8_t chunk13_3_ui_qol_no_cancel_via_lmb[] = {
    /*0x705e9*/ 0x3c,
};
const uint8_t replace13_3_ui_qol_no_cancel_via_lmb[] = {
    /*0x705e9*/ 0x4e,
};

const int chunk14_1_start = 0x705f2;
const int chunk14_1_end = 0x705f5;
const uint8_t chunk14_1_ui_qol_extra_key_bindings_scrap[] = {
    /*0x705f2*/ 0xb8, 0x5a, 0x48,
};
const uint8_t replace14_1_ui_qol_extra_key_bindings_scrap[] = {
    /*0x705f2*/ 0x90, 0xeb, 0x33,
};
const int chunk14_2_start = 0x70628;
const int chunk14_2_end = 0x70631;
const uint8_t chunk14_2_ui_qol_extra_key_bindings_scrap[] = {
    /*0x70628*/ 0xff,   /* NOTE: this requires fix@13 */
    /*0x70629*/ 0x50,
    /*0x7062a*/ 0xb8, 0xf5, 0x48,
    /*0x7062d*/ 0x50,
    /*0x7062e*/ 0xb8, 0xc7, 0x00,
};
const uint8_t replace14_2_ui_qol_extra_key_bindings_scrap[] = {
    /*0x70628*/ 0xb8, 0x84, 0x6d,
    /*0x7062b*/ 0x03, 0xc6,
    /*0x7062d*/ 0xeb, 0xc6,
    /*0x7062f*/ 0x90, 0x90,
};

const int chunk15_1_start = 0x7bf0a;
const int chunk15_1_end = 0x7bf16;
const uint8_t chunk15_1_ui_qol_extra_key_bindings_races[] = {
    /*0x7bf0a*/ 0x33, 0xf6,
    /*0x7bf0c*/ 0xeb, 0x17,
    /*0x7bf0e*/ 0x8b, 0xde,
    /*0x7bf10*/ 0xd1, 0xe3,
    /*0x7bf12*/ 0x8b, 0x87, 0xc2, 0xd1,
};
const uint8_t replace15_1_ui_qol_extra_key_bindings_races[] = {
    /*0x7bf0a*/ 0xeb, 0x20,
    /*0x7bf0c*/ 0xb8, 0x84, 0x6d,
    /*0x7bf0f*/ 0x03, 0xc6,
    /*0x7bf11*/ 0xe9, 0x8d, 0x01,
    /*0x7bf14*/ 0x90, 0x90,
};
const int chunk15_2_start = 0x7c09e;
const int chunk15_2_end = 0x7c0a1;
const uint8_t chunk15_2_ui_qol_extra_key_bindings_races[] = {
    /*0x7c09e*/ 0xb8, 0xb0, 0x55,
};
const uint8_t replace15_2_ui_qol_extra_key_bindings_races[] = {
    /*0x7c09e*/ 0xe9, 0x6b, 0xfe,
};

const int chunk16_1_start = 0x637af;
const int chunk16_1_end = 0x637b2;
const int chunk16_2_start = 0x637d2;
const int chunk16_2_end = 0x637d5;
const int chunk16_3_start = 0x637f5;
const int chunk16_3_end = 0x637f8;
const int chunk16_4_start = 0x63818;
const int chunk16_4_end = 0x6381b;
const int chunk16_5_start = 0x6383b;
const int chunk16_5_end = 0x6383e;
const int chunk16_6_start = 0x6385e;
const int chunk16_6_end = 0x63861;
const uint8_t chunk16_ui_qol_extra_key_bindings_tech[] = {
    /*0x637af*/ 0xb8, 0x0a, 0x3c,
};
const uint8_t replace16_1_ui_qol_extra_key_bindings_tech[] = {
    /*0x637af*/ 0xb8, 0x84, 0x6d,
};
const uint8_t replace16_2_ui_qol_extra_key_bindings_tech[] = {
    /*0x637d2*/ 0xb8, 0x85, 0x6d,
};
const uint8_t replace16_3_ui_qol_extra_key_bindings_tech[] = {
    /*0x637f5*/ 0xb8, 0x86, 0x6d,
};
const uint8_t replace16_4_ui_qol_extra_key_bindings_tech[] = {
    /*0x63818*/ 0xb8, 0x87, 0x6d,
};
const uint8_t replace16_5_ui_qol_extra_key_bindings_tech[] = {
    /*0x6383b*/ 0xb8, 0x88, 0x6d,
};
const uint8_t replace16_6_ui_qol_extra_key_bindings_tech[] = {
    /*0x6385e*/ 0xb8, 0x89, 0x6d,
};

const int chunk17_1_start = 0x7f500;
const int chunk17_1_end = 0x7f503;
const int chunk17_2_start = 0x7f523;
const int chunk17_2_end = 0x7f526;
const int chunk17_3_start = 0x7f546;
const int chunk17_3_end = 0x7f549;
const uint8_t chunk17_ui_qol_extra_key_bindings_election[] = {
    /*0x7f500*/ 0xb8, 0x0e, 0x57,
};
const uint8_t replace17_1_ui_qol_extra_key_bindings_election[] = {
    /*0x7f500*/ 0xb8, 0x84, 0x6d,
};
const uint8_t replace17_2_ui_qol_extra_key_bindings_election[] = {
    /*0x7f523*/ 0xb8, 0x85, 0x6d,
};
const uint8_t replace17_3_ui_qol_extra_key_bindings_election[] = {
    /*0x7f546*/ 0xb8, 0x83, 0x6d,
};

const int chunk18_1_start = 0x7e112;
const int chunk18_1_end = 0x7e116;
const uint8_t chunk18_1_ui_fix_empirereport_environ[] = {
    /*0x7e112*/ 0xdc, 0x02,
    /*0x7e114*/ 0x98,
    /*0x7e115*/ 0x40,
};
const uint8_t replace18_1_ui_fix_empirereport_environ[] = {
    /*0x7e112*/ 0x90, 0x03,
    /*0x7e114*/ 0x98,
    /*0x7e115*/ 0x90,
};

const int chunk18_2_start = 0x7e15e;
const int chunk18_2_end = 0x7e16c;
const uint8_t chunk18_2_ui_fix_empirereport_environ[] = {
    /*0x7e15e*/ 0xc6, 0x07, 0x00,
    /*0x7e161*/ 0xb8, 0xba, 0x56,
    /*0x7e164*/ 0x50,
    /*0x7e165*/ 0x8d, 0x46, 0x98,
    /*0x7e168*/ 0x50,
    /*0x7e169*/ 0x9a, 0x40, 0x2f,
};
const uint8_t replace18_2_ui_fix_empirereport_environ[] = {
    /*0x7e15e*/ 0x83, 0xc3, 0x08,
    /*0x7e161*/ 0x83, 0xFF, 0x03,
    /*0x7e164*/ 0x90,
    /*0x7e165*/ 0x75, 0x03,
    /*0x7e167*/ 0xc6, 0x07, 0x00,
    /*0x7e16a*/ 0xeb, 0x04,
    /* Note: This jump bypasses code that is broken by the relocation table. */
};

const int chunk19_start = 0x6c80a;
const int chunk19_end = 0x6c80b;
const uint8_t chunk19_ui_fix_starmap_oor_msg[] = {
    /*0x6c80a*/ 0x07,
};
const uint8_t replace19_ui_fix_starmap_oor_msg[] = {
    /*0x6c80a*/ 0x06,
};

const int chunk20_start = 0x15ea5;
const int chunk20_end = 0x15ea7;
const uint8_t chunk20_ui_qol_prebattle_no_setfocus[] = {
    /*0x15ea5*/ 0xff, 0x76,
};
const uint8_t replace20_ui_qol_prebattle_no_setfocus[] = {
    /*0x15ea5*/ 0xeb, 0x07,
};

const int chunk21_start = 0x69ddd;
const int chunk21_end = 0x69de0;
const uint8_t chunk21_game_ai_fix_cancelled_threat[] = {
    /*0x69ddd*/ 0xe9, 0x7c, 0x02,
};
const uint8_t replace21_game_ai_fix_cancelled_threat[] = {
    /*0x69ddd*/ 0xe9, 0x93, 0x02,
};

const int chunk22_start = 0x76dab;
const int chunk22_end = 0x76dac;
const uint8_t chunk22_ui_fix_slider_text_ind[] = {
    /*0x76dab*/ 0x7f,
};
const uint8_t replace22_ui_fix_slider_text_ind[] = {
    /*0x76dab*/ 0x7d,
};

const int chunk101_start = 0x114c9;
const int chunk101_end = 0x114ca;
const uint8_t chunk101_game_fix_orbital_torpedo[] = {
    /*0x114c9*/ 0x75,
};
const uint8_t replace101_game_fix_orbital_torpedo[] = {
    /*0x114c9*/ 0x74,
};

const int chunk102_1_start = 0x115fa;
const int chunk102_1_end = 0x115fb;
const int chunk102_2_start = 0x8271b;
const int chunk102_2_end = 0x8271c;
const uint8_t chunk102_game_fix_orbital_weap_any[] = {
    /*0x115fa*/ 0x3d,
};
const uint8_t replace102_game_fix_orbital_weap_any[] = {
    /*0x115fa*/ 0x40,
};

const int chunk103_start = 0x5b0ac;
const int chunk103_end = 0x5b0ae;
const uint8_t chunk103_game_fix_bt_min_missile_hit[] = {
    /*0x5b0ac*/ 0x7e, 0x03,
};
const uint8_t replace103_game_fix_bt_min_missile_hit[] = {
    /*0x5b0ac*/ 0x7c, 0x03,
};

const int chunk104_start = 0xcb31;
const int chunk104_end = 0xcb35;
const uint8_t chunk104_game_fix_broken_garbage[] = {
    /*0xcb31*/ 0x0e,
    /*0xcb32*/ 0xe8, 0xb8, 0x29,
};

const int chunk105_1_start = 0xf4ed;
const int chunk105_1_end = 0xf504;
const uint8_t chunk105_1_game_fix_dead_ai_designs_ships[] = {
    /*0xf4ed*/ 0x55,
    /*0xf4ee*/ 0x8b, 0xec,
    /*0xf4f0*/ 0x83, 0xec, 0x1a,
    /*0xf4f3*/ 0x56,
    /*0xf4f4*/ 0x57,
    /*0xf4f5*/ 0x33, 0xf6,
    /*0xf4f7*/ 0xeb, 0x0e,
    /*0xf4f9*/ 0x8b, 0xde,
    /*0xf4fb*/ 0xd1, 0xe3,
    /*0xf4fd*/ 0x8d, 0x46, 0xf2,
    /*0xf500*/ 0x03, 0xd8,
    /*0xf502*/ 0xc7, 0x07,
};
const uint8_t replace105_1_game_fix_dead_ai_designs_ships[] = {
    /*0xf4ed*/ 0x8b, 0xc6,  /* NOTE: this requires fix@104 */
    /*0xf4ef*/ 0xd1, 0xe0,  /* Introduces proc si_is_alive. */
    /*0xf4f1*/ 0xc4, 0x1e, 0x8c, 0x77,
    /*0xf4f5*/ 0x03, 0xd8,
    /*0xf4f7*/ 0x33, 0xc0,
    /*0xf4f9*/ 0x26, 0x83, 0xbf, 0xa0, 0x00, 0xff,
    /*0xf4ff*/ 0x74, 0x02,
    /*0xf501*/ 0xb0, 0x01,
    /*0xf503*/ 0xcb,
};

const int chunk105_2_start = 0xf95a;
const int chunk105_2_end = 0xf96a;
const uint8_t chunk105_2_game_fix_dead_ai_designs_ships[] = {
    /*0xf95a*/ 0x8b, 0xc6,
    /*0xf95c*/ 0xba, 0xd4, 0x0d,
    /*0xf95f*/ 0xf7, 0xea,
    /*0xf961*/ 0x8b, 0xd8,
    /*0xf963*/ 0x83, 0xbf, 0x78, 0x78, 0x00,
    /*0xf968*/ 0x7f, 0x07,
};
const uint8_t replace105_2_game_fix_dead_ai_designs_ships[] = {
    /*0xf95a*/ 0x83, 0xbf, 0x78, 0x78, 0x00,
    /*0xf95f*/ 0x7f, 0x10,
    /*0xf961*/ 0x90,
    /*0xf962*/ 0x0e,
    /*0xf963*/ 0xe8, 0x87, 0xfb,
    /*0xf966*/ 0x3c, 0x00,
    /*0xf968*/ 0x74, 0x07,
};

const int chunk106_1_start = 0x5bf42;
const int chunk106_1_end = 0x5bf4e;
const uint8_t chunk106_1_game_fix_oracle_interface[] = {
    /*0x5bf42*/ 0xba, 0x30, 0x00,
    /*0x5bf45*/ 0xf7, 0xea,
    /*0x5bf47*/ 0x8b, 0xd8,
    /*0x5bf49*/ 0x8b, 0x9f, 0x7c, 0x0e,
    /*0x5bf4d*/ 0x43,
};
const uint8_t replace106_1_game_fix_oracle_interface[] = {
    /*0x5bf42*/ 0x90,
    /*0x5bf43*/ 0x50,
    /*0x5bf44*/ 0x57,
    /*0x5bf45*/ 0x2e, 0xff, 0x1e, 0x70, 0x14,
    /*0x5bf4a*/ 0x59,   /* NOTE: Using a pointer to a far call */
    /*0x5bf4b*/ 0x59,
    /*0x5bf4c*/ 0x8b, 0xd8,
};

const int chunk107_start = 0x862f8;
const int chunk107_end = 0x862fc;
const uint8_t chunk107_game_ai_fix_final_war_fronts[] = {
    /*0x862f8*/ 0x83, 0x7e, 0x06, 0x00,
};
const uint8_t replace107_game_ai_fix_final_war_fronts[] = {
    /*0x862f8*/ 0x90, 0x90,
    /*0x862fa*/ 0x0b, 0xff,
};

const int chunk108_start = 0x85733;
const int chunk108_end = 0x85743;
const uint8_t chunk108_game_ai_fix_4th_colony_curse[] = {
    /*0x85733*/ 0x8b, 0xc7,
    /*0x85735*/ 0xd1, 0xe0,
    /*0x85737*/ 0x03, 0xd8,
    /*0x85739*/ 0x26, 0x83, 0xbf, 0x5c, 0x04, 0x03,
    /*0x8573f*/ 0x76, 0x02,
    /*0x85741*/ 0xeb, 0x9c,
};
const uint8_t replace108_game_ai_fix_4th_colony_curse[] = {
    /*0x85733*/ 0x03, 0xdf,
    /*0x85735*/ 0x03, 0xdf,
    /*0x85737*/ 0x26, 0x83, 0xbf, 0x5c, 0x04, 0x03,
    /*0x8573d*/ 0x76, 0x04,
    /*0x8573f*/ 0x90, 0xe9,
    /*0x85741*/ 0xe9, 0x00,
};

const int chunk109_1_start = 0x15921;
const int chunk109_1_end = 0x15926;
const uint8_t chunk109_1_game_fix_guardian_repair[] = {
    /*0x15921*/ 0x83, 0x3e, 0x88, 0xcb, 0x03,
};
const uint8_t replace109_1_game_fix_guardian_repair[] = {
    /*0x15921*/ 0x83, 0x3e, 0x88, 0xcb, 0x04,
};

const int chunk109_2_start = 0x1595e;
const int chunk109_2_end = 0x15963;
const uint8_t chunk109_2_game_fix_guardian_repair[] = {
    /*0x1595e*/ 0x83, 0x3e, 0x88, 0xcb, 0x03,
};
const uint8_t replace109_2_game_fix_guardian_repair[] = {
    /*0x1595e*/ 0x83, 0x3e, 0x88, 0xcb, 0x04,
};

const int chunk201_start = 0x88c8;
const int chunk201_end = 0x892e;
const uint8_t chunk201_game_fix_sg_maint_overflow[] = {
    /*0x88c8*/ 0x8b, 0x46, 0xfc,
    /*0x88cb*/ 0xba, 0xd4, 0x0d,
    /*0x88ce*/ 0xf7, 0xea,
    /*0x88d0*/ 0x8b, 0xd8,
    /*0x88d2*/ 0x81, 0xbf, 0x52, 0x7b, 0x00, 0x7d,
    /*0x88d8*/ 0x76, 0x10,
    /*0x88da*/ 0x8b, 0x46, 0xfc,
    /*0x88dd*/ 0xba, 0xd4, 0x0d,
    /*0x88e0*/ 0xf7, 0xea,
    /*0x88e2*/ 0x8b, 0xd8,
    /*0x88e4*/ 0xc7, 0x87, 0x52, 0x7b, 0x00, 0x7d,

    /*0x88ea*/ 0x33, 0xff,
    /*0x88ec*/ 0xeb, 0x3a,
    /*0x88ee*/ 0x8b, 0xc7,
    /*0x88f0*/ 0xba, 0xb8, 0x00,
    /*0x88f3*/ 0xf7, 0xea,
    /*0x88f5*/ 0xc4, 0x1e, 0x72, 0xcb,
    /*0x88f9*/ 0x03, 0xd8,
    /*0x88fb*/ 0x26, 0x83, 0x7f, 0x64, 0x00,
    /*0x8900*/ 0x74, 0x25,
    /*0x8902*/ 0x8b, 0xc7,
    /*0x8904*/ 0xba, 0xb8, 0x00,
    /*0x8907*/ 0xf7, 0xea,
    /*0x8909*/ 0xc4, 0x1e, 0x72, 0xcb,
    /*0x890d*/ 0x03, 0xd8,
    /*0x890f*/ 0x26, 0x8b, 0x47, 0x36,
    /*0x8913*/ 0x3b, 0x46, 0xfc,
    /*0x8916*/ 0x75, 0x0f,
    /*0x8918*/ 0x8b, 0x46, 0xfc,
    /*0x891b*/ 0xba, 0xd4, 0x0d,
    /*0x891e*/ 0xf7, 0xea,
    /*0x8920*/ 0x8b, 0xd8,
    /*0x8922*/ 0x83, 0x87, 0x52, 0x7b, 0x64,
    /*0x8927*/ 0x47,
    /*0x8928*/ 0x3b, 0x3e, 0x86, 0xcb,
    /*0x892c*/ 0x7c, 0xc0,
};
const uint8_t replace201_game_fix_sg_maint_overflow[] = {
    /*0x88ea*/ 0x33, 0xff,
    /*0x88ec*/ 0xeb, 0x3a,
    /*0x88ee*/ 0x8b, 0xc7,
    /*0x88f0*/ 0xba, 0xb8, 0x00,
    /*0x88f3*/ 0xf7, 0xea,
    /*0x88f5*/ 0xc4, 0x1e, 0x72, 0xcb,
    /*0x88f9*/ 0x03, 0xd8,
    /*0x88fb*/ 0x26, 0x83, 0x7f, 0x64, 0x00,
    /*0x8900*/ 0x74, 0x25,
    /*0x8902*/ 0x8b, 0xc7,
    /*0x8904*/ 0xba, 0xb8, 0x00,
    /*0x8907*/ 0xf7, 0xea,
    /*0x8909*/ 0xc4, 0x1e, 0x72, 0xcb,
    /*0x890d*/ 0x03, 0xd8,
    /*0x890f*/ 0x26, 0x8b, 0x47, 0x36,
    /*0x8913*/ 0x3b, 0x46, 0xfc,
    /*0x8916*/ 0x75, 0x0f,
    /*0x8918*/ 0x8b, 0x46, 0xfc,
    /*0x891b*/ 0xba, 0xd4, 0x0d,
    /*0x891e*/ 0xf7, 0xea,
    /*0x8920*/ 0x8b, 0xd8,
    /*0x8922*/ 0x83, 0x87, 0x52, 0x7b, 0x64,
    /*0x8927*/ 0x47,
    /*0x8928*/ 0x3b, 0x3e, 0x86, 0xcb,
    /*0x892c*/ 0x7c, 0xc0,

    /*0x88c8*/ 0x8b, 0x46, 0xfc,
    /*0x88cb*/ 0xba, 0xd4, 0x0d,
    /*0x88ce*/ 0xf7, 0xea,
    /*0x88d0*/ 0x8b, 0xd8,
    /*0x88d2*/ 0x81, 0xbf, 0x52, 0x7b, 0x00, 0x7d,
    /*0x88d8*/ 0x76, 0x10,
    /*0x88da*/ 0x8b, 0x46, 0xfc,
    /*0x88dd*/ 0xba, 0xd4, 0x0d,
    /*0x88e0*/ 0xf7, 0xea,
    /*0x88e2*/ 0x8b, 0xd8,
    /*0x88e4*/ 0xc7, 0x87, 0x52, 0x7b, 0x00, 0x7d,
};

const bin_chunk_t patch_1_3a_data[] = {
    {chunk0_ui_qol_starmap_no_qmark_cursor, NULL, chunk0_start, chunk0_end},
    {chunk1_0_game_fix_space_scanners, NULL, chunk1_0_start, chunk1_0_end},
    {chunk1_1_game_fix_space_scanners, NULL, chunk1_1_start, chunk1_1_end},
    {chunk1_2_game_fix_space_scanners, subst1_2_game_fix_space_scanners, chunk1_2_start, chunk1_2_end},
    {chunk2_ui_fix_starmap_background, chunk3_ui_fix_starmap_background, chunk2_0_off, chunk2_0_off + 4},
    {chunk2_ui_fix_starmap_background, chunk3_ui_fix_starmap_background, chunk2_1_off, chunk2_1_off + 4},
    {chunk2_ui_fix_starmap_background, chunk3_ui_fix_starmap_background, chunk2_2_off, chunk2_2_off + 4},
    {chunk2_ui_fix_starmap_background, chunk3_ui_fix_starmap_background, chunk2_3_off, chunk2_3_off + 4},
    {chunk3_ui_fix_starmap_background, chunk2_ui_fix_starmap_background, chunk3_0_off, chunk3_0_off + 4},
    {chunk3_ui_fix_starmap_background, chunk2_ui_fix_starmap_background, chunk3_1_off, chunk3_1_off + 4},
    {chunk3_ui_fix_starmap_background, chunk2_ui_fix_starmap_background, chunk3_2_off, chunk3_2_off + 4},
    {chunk3_ui_fix_starmap_background, chunk2_ui_fix_starmap_background, chunk3_3_off, chunk3_3_off + 4},
    {chunk4_ui_fix_spy_cost, NULL, chunk4_start, chunk4_end},
    {chunk5_ui_fix_tech_complete_probability, subst5_ui_fix_tech_complete_probability, chunk5_start, chunk5_end},
    {chunk6_ui_qol_starmap_msg_pos, subst6_ui_qol_starmap_msg_pos, chunk6_start, chunk6_end},
    {chunk7_game_ai_fix_spy_hiding, subst7_game_ai_fix_spy_hiding, chunk7_start, chunk7_end},
    {chunk8_main_allow_direct_launch, subst8_main_allow_direct_launch, chunk8_start, chunk8_end},
    {chunk9_ui_qol_extra_key_bindings_space, subst9_ui_qol_extra_key_bindings_space, chunk9_1_start, chunk9_1_end},
    {chunk9_ui_qol_extra_key_bindings_space, subst9_ui_qol_extra_key_bindings_space, chunk9_2_start, chunk9_2_end},
    {chunk10_game_fix_sg_finished, replace10_game_fix_sg_finished, chunk10_1_start, chunk10_1_end},
    {chunk10_game_fix_sg_finished, replace10_game_fix_sg_finished, chunk10_2_start, chunk10_2_end},
    {chunk11_game_fix_max_factories, replace11_game_fix_max_factories, chunk11_1_start, chunk11_1_end},
    {chunk11_game_fix_max_factories, replace11_game_fix_max_factories, chunk11_2_start, chunk11_2_end},
    {chunk12_ui_fix_planet_list_pos, replace12_ui_fix_planet_list_pos, chunk12_start, chunk12_end},
    {chunk13_1_ui_qol_no_cancel_via_lmb, replace13_1_ui_qol_no_cancel_via_lmb, chunk13_1_start, chunk13_1_end},
    {chunk13_2_ui_qol_no_cancel_via_lmb, replace13_2_ui_qol_no_cancel_via_lmb, chunk13_2_start, chunk13_2_end},
    {chunk13_3_ui_qol_no_cancel_via_lmb, replace13_3_ui_qol_no_cancel_via_lmb, chunk13_3_start, chunk13_3_end},
    {chunk14_1_ui_qol_extra_key_bindings_scrap, replace14_1_ui_qol_extra_key_bindings_scrap, chunk14_1_start, chunk14_1_end},
    {chunk14_2_ui_qol_extra_key_bindings_scrap, replace14_2_ui_qol_extra_key_bindings_scrap, chunk14_2_start, chunk14_2_end},
    {chunk15_1_ui_qol_extra_key_bindings_races, replace15_1_ui_qol_extra_key_bindings_races, chunk15_1_start, chunk15_1_end},
    {chunk15_2_ui_qol_extra_key_bindings_races, replace15_2_ui_qol_extra_key_bindings_races, chunk15_2_start, chunk15_2_end},
    {chunk16_ui_qol_extra_key_bindings_tech, replace16_1_ui_qol_extra_key_bindings_tech, chunk16_1_start, chunk16_1_end},
    {chunk16_ui_qol_extra_key_bindings_tech, replace16_2_ui_qol_extra_key_bindings_tech, chunk16_2_start, chunk16_2_end},
    {chunk16_ui_qol_extra_key_bindings_tech, replace16_3_ui_qol_extra_key_bindings_tech, chunk16_3_start, chunk16_3_end},
    {chunk16_ui_qol_extra_key_bindings_tech, replace16_4_ui_qol_extra_key_bindings_tech, chunk16_4_start, chunk16_4_end},
    {chunk16_ui_qol_extra_key_bindings_tech, replace16_5_ui_qol_extra_key_bindings_tech, chunk16_5_start, chunk16_5_end},
    {chunk16_ui_qol_extra_key_bindings_tech, replace16_6_ui_qol_extra_key_bindings_tech, chunk16_6_start, chunk16_6_end},
    {chunk17_ui_qol_extra_key_bindings_election, replace17_1_ui_qol_extra_key_bindings_election, chunk17_1_start, chunk17_1_end},
    {chunk17_ui_qol_extra_key_bindings_election, replace17_2_ui_qol_extra_key_bindings_election, chunk17_2_start, chunk17_2_end},
    {chunk17_ui_qol_extra_key_bindings_election, replace17_3_ui_qol_extra_key_bindings_election, chunk17_3_start, chunk17_3_end},
    {chunk18_1_ui_fix_empirereport_environ, replace18_1_ui_fix_empirereport_environ, chunk18_1_start, chunk18_1_end},
    {chunk18_2_ui_fix_empirereport_environ, replace18_2_ui_fix_empirereport_environ, chunk18_2_start, chunk18_2_end},
    {chunk19_ui_fix_starmap_oor_msg, replace19_ui_fix_starmap_oor_msg, chunk19_start, chunk19_end},
    {chunk20_ui_qol_prebattle_no_setfocus, replace20_ui_qol_prebattle_no_setfocus, chunk20_start, chunk20_end},
    {chunk21_game_ai_fix_cancelled_threat, replace21_game_ai_fix_cancelled_threat, chunk21_start, chunk21_end},
    {chunk22_ui_fix_slider_text_ind, replace22_ui_fix_slider_text_ind, chunk22_start, chunk22_end},

    {chunk101_game_fix_orbital_torpedo, replace101_game_fix_orbital_torpedo, chunk101_start, chunk101_end},
    {chunk102_game_fix_orbital_weap_any, replace102_game_fix_orbital_weap_any, chunk102_1_start, chunk102_1_end},
    {chunk102_game_fix_orbital_weap_any, replace102_game_fix_orbital_weap_any, chunk102_2_start, chunk102_2_end},
    {chunk103_game_fix_bt_min_missile_hit, replace103_game_fix_bt_min_missile_hit, chunk103_start, chunk103_end},
    {chunk104_game_fix_broken_garbage, NULL, chunk104_start, chunk104_end},
    {chunk105_1_game_fix_dead_ai_designs_ships, replace105_1_game_fix_dead_ai_designs_ships, chunk105_1_start, chunk105_1_end},
    {chunk105_2_game_fix_dead_ai_designs_ships, replace105_2_game_fix_dead_ai_designs_ships, chunk105_2_start, chunk105_2_end},
    {chunk106_1_game_fix_oracle_interface, replace106_1_game_fix_oracle_interface, chunk106_1_start, chunk106_1_end},
    {chunk107_game_ai_fix_final_war_fronts, replace107_game_ai_fix_final_war_fronts, chunk107_start, chunk107_end},
    {chunk108_game_ai_fix_4th_colony_curse, replace108_game_ai_fix_4th_colony_curse, chunk108_start, chunk108_end},
    {chunk109_1_game_fix_guardian_repair, replace109_1_game_fix_guardian_repair, chunk109_1_start, chunk109_1_end},
    {chunk109_2_game_fix_guardian_repair, replace109_2_game_fix_guardian_repair, chunk109_2_start, chunk109_2_end},

    {chunk201_game_fix_sg_maint_overflow, replace201_game_fix_sg_maint_overflow, chunk201_start, chunk201_end},
    {NULL, NULL, 0, 0},
};

const bin_patch_t patch_1_3a[] = {
    {"STARMAP.EXE", patch_1_3a_data},
    {NULL, NULL},
};
