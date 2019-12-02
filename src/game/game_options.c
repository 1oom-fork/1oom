#include "game_options.h"

optdescr_t gameopt_descr[GAMEOPTS] = {
  { 2, 0, "Rules", { "unlocked", "locked" },
    "Locks rules so they cannot be changed in the Game menu and disables cheat codes."
  },
  { 3, 1, "RNG", { "random", "deterministic", "fixed" },
    "RNG settings: random, deterministic (default), fixed (deterministic and reset to a fixed value after each player turn)"
  },
  { 4, 0, "Council", { "2/3", "late", "Orion", "none" },
    "Galactic Council setting: 2/3 (default), late (every 25 years from 2400 on if 3/4 of planets are settled), after Orion is settled, never"
  },
  { 3, 0, "Guardian", { "normal", "weak", "weaker" },
    "Guardian setting: normal (default), weak (less weapons and DEF, no ARS/ADC), weaker (also less shields and ATT)"
  },
  { 3, 0, "Retreat", { "may stay", "flee", "rearm" },
    "Retreating fleets: may stay in the system (default), flee (have to retreat to next friendly planet), rearm (flee if having spent ammo)"
  },
  { 3, 0, "Spec Wars", { "allowed", "declare", "war only" },
    "Asking for a Declaration of War on another Race is allowed (default), declares war if accepted or is banned unless already at war with target."
  },
  { 4, 0, "Battle", { "exploits", "no baiting", "no yoyo", "no bait/yoyo" },
    "No baiting lets AI retreat if bases are untouchable. No yoyo will not evade ship missiles if stronger bases are present."
  },
  { 4, 0, "Nebula speed", { "half", "warp 1", "step", "full" },
    "Set nebula speed to half (default) like in MOO v1.3, to warp 1 as claimed in the OSG, to a single 1/2 parsec step or to full speed."
  },
  { 4, 0, "Enforce NAP", { "no", "ships", "trans", "truce" },
    "Non-Aggression Pact enforcement: none (default), return ships, return ships and transports, also show and enforce truces for AI and Player."
  },
  { 4, 0, "Threats", { "normal", "ultimatum", "smart", "off" },
    "Ultimatum means war when denied. Smart discers valid complaints from extortion and also takes relations, strength and treaties into account."
  }
};

optdescr_t newopt_descr[NEWOPTS] = {
  { 4, 1, "Density", { "high", "normal", "low", "sparse" }, "" },
  { 5, 2, "Gaps", { "narrow", "close", "medium", "wide", "vast" }, "" },
  { 4, 0, "Cluster", { "one per player", "pls + 1", "pls + 2", "pls + 3" }, "" },
  { 6, 3, "Nebulas", { "random", "none", "rare", "common", "frequent", "max" }, "" },
  { 4, 0, "Homeworlds", { "map", "reroll", "distant", "fair" }, "" },
  { 4, 0, "Start", { "random", "good", "average", "bad" }, "" },
  { 3, 1, "Planet Size", { "small", "medium", "large" }, "" },
  { 3, 1, "Environment", { "hostile", "moderate", "fertile" }, "" },
  { 3, 1, "Minerals", { "poor", "average", "rich" }, "" },
  { 4, 3, "Ultra Planets", { "none", "no urich", "no upoor", "yes" }, "" },
  { 4, 2, "Artefacts", { "none", "rare", "normal", "frequent" }, "" },
  { 2, 1, "Astroids", { "none", "yes" }, "" }
};
