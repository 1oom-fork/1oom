1oom
====

1oom is a Master of Orion (1993) game engine recreation.
1oom is Free Software (GPLv2), see COPYING.


Installation
============

1oom requires a copy of the Master of Orion (v1.3) LBX files.

Windows
-------

Simply copy the EXE and DLLs (if any) to your MOO1 directory.

Linux
-----

1oom requires the following libraries:

- SDL (libsdl1.2):
http://www.libsdl.org
- SDL_mixer (libsdl-mixer1.2):
http://www.libsdl.org/projects/SDL_mixer/

Also recommended:

- libsamplerate:
http://www.mega-nerd.com/libsamplerate/

Check your distribution's package manager or the library
website on how to install them.


Configuration
=============

1oom is configured via command-line arguments or editing the
configuration file.

User Directory
--------------

1oom creates a User directory with all the user savegames and
options in one of the following paths:

- <game directory> (win32)
- $XDG_CONFIG_HOME/1oom (Linux)
- ~/.config/1oom (Linux)


Development
===========

See INSTALL and HACKING for more information.


Executables
===========

1oom consists of a few executables:

- 1oom_classic_sdl1 (the 1993 UI)
- 1oom_lbxview_sdl1 (for viewing LBX files)
- 1oom_saveconv     (for converting save game files)

See doc/ for more documentation.

1oom_classic_sdl1
-----------------

Middle click or Ctrl-F10 to grab/ungrab input.
Ctrl-Esc to quit (without saving).
Ctrl-Plus/Minus to adjust sfx volume.
Ctrl-Shift-Plus/Minus to adjust music volume.
Otherwise it controls like the original.


Abbreviations
=============

The following abbreviations appear in the documentation and code:

- MOO1: Master of Orion 1 as represented by the v1.3 DOS binaries
- OSG: The Official Strategy Guide (ISBN 1-55958-507-2)


Acknowledgements
================

Most of the credit for this software belongs to the progammer who authored
[1oom v1.0](https://kilgoretroutmaskreplicant.gitlab.io/plain-html)
under the pseudonym Kilgore Trout Mask Replicant. Thank you for
creating this and publishing it free and open source!

The original game Master of Orion was developed  by Simtex Software and
published 1993 by MicroProse. Thanks for the great game!

Thanks to Alan Emrich and Tom Hughes for documenting the game mechanics and AI
decision making in great detail in the official strategy guide.

Ideas and text snippets have been taken from kyrub's unofficial patch 1.40m
Readme. Thanks for the patch.

Thanks to [shikadi.net](https://www.shikadi.net) for documenting the
[music format](https://moddingwiki.shikadi.net/wiki/XMI_Format).

Thanks to [CivFanatics forum](https://forums.civfanatics.com) user sargon0 for [partial save game format info](
https://forums.civfanatics.com/threads/moo-save-file-layout.275055/).

Thanks to those who contributed code, ideas or bug reports.

Some code has been pilfered from
[Chocolate Doom](https://github.com/chocolate-doom/chocolate-doom)
and [VICE](https://vice-emu.sourceforge.io/).

[HACKING](HACKING) and [PHILOSOPHY](PHILOSOPHY) are based on Chocolate Doom.
