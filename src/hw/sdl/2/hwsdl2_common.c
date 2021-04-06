#include "config.h"
#include "SDL.h"

/* We'd like to compile the common sources into separate object files for SDL 1 and 2 because there may be different compile-time preprocessor defines. this file exists to make that happen */

#ifdef HAVE_SDL2MIXER
#define HAVE_SDLMIXER
#include "SDL_mixer.h"
#include "SDL_rwops.h"
#define USE_SFX_INIT_THREAD
#define HWSDLX_CreateThread(_func_) SDL_CreateThread(_func_, "SFX init", 0)
#endif /* HAVE_SDL2MIXER */

#include "hwsdl_audio.c"

#include "hwsdl_mouse.c"
#include "hwsdl_aspect.c"

