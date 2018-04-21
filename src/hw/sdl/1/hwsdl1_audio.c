#include "config.h"

#ifdef HAVE_SDLMIXER1
#define HAVE_SDLMIXER
#include "SDL.h"
#include "SDL_mixer.h"
#include "SDL_rwops.h"
#endif /* HAVE_SDLMIXER1 */

#include "hwsdl_audio.c"
