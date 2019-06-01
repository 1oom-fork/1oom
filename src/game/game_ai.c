#include "config.h"

#include "game_ai.h"
#include "game_ai_classic.h"
#include "game_ai_muxer.h"
#include "game_ai_stub.h"
#include "game.h"
#include "game_aux.h"

/* -------------------------------------------------------------------------- */

const struct game_ai_s *game_ai = &game_ai_classic;

const struct game_ai_s *const game_ais[GAME_AI_NUM] = {
    [GAME_AI_CLASSIC]     = &game_ai_classic,
    [GAME_AI_CLASSICPLUS] = &game_ai_classicplus,
    [GAME_AI_MUXER]       = &game_ai_muxer,
    [GAME_AI_STUB]        = &game_ai_stub
};
