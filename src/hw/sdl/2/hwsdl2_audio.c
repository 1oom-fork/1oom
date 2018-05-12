#include "config.h"

#ifdef HAVE_SDL2MIXER
#define HAVE_SDLMIXER
#include "SDL.h"
#include "SDL_mixer.h"
#include "SDL_rwops.h"
#endif /* HAVE_SDL2MIXER */

#include "hwsdl_audio.c"
