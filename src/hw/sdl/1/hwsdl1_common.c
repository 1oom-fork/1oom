#include "config.h"
#include "SDL.h"

#ifdef HAVE_SDL1MIXER
#define HAVE_SDLMIXER
#include "SDL_mixer.h"
#include "SDL_rwops.h"
#define USE_SFX_INIT_THREAD
#define HWSDLX_CreateThread(_func_) SDL_CreateThread(_func_, 0)
#endif /* HAVE_SDL1MIXER */
#include "hwsdl_audio.c"

#include "hwsdl_mouse.c"
#include "hwsdl_aspect.c"

