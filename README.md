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

Unix (Linux)
------------

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

See CONFIGURATION.


Development
===========

See INSTALL and HACKING for more information.


Controls
========

- Middle click or Ctrl-F10 to grab/ungrab input.
- Ctrl-Esc to quit (without saving).
- Ctrl-Plus/Minus to adjust sfx volume.
- Ctrl-Shift-Plus/Minus to adjust music volume.

Otherwise it controls like the original.


Abbreviations
=============

The following abbreviations appear in the documentation and code:

- MOO1: Master of Orion 1 as represented by the v1.3 DOS binaries
- OSG: The Official Strategy Guide (ISBN 1-55958-507-2)


Acknowledgements
================

Most of the credit for this software belongs to the programmer who authored
[1oom v1.0](https://kilgoretroutmaskreplicant.gitlab.io/plain-html) under the pseudonym Kilgore Trout Mask Replicant. Thank you for
creating this and publishing it free and open source!

The original game Master of Orion was developed  by Simtex Software and
published in 1993 by MicroProse. Thanks for the great game!

Thanks to Alan Emrich and Tom Hughes for documenting the game mechanics and AI
decision making in great detail in the official strategy guide.

Ideas and text snippets have been taken from kyrub's unofficial patch 1.40m
Readme. Thanks for the patch.

Thanks to [shikadi.net](https://www.shikadi.net) for documenting the [music format](https://moddingwiki.shikadi.net/wiki/XMI_Format).

Thanks to [CivFanatics forum](https://forums.civfanatics.com) user sargon0 for [partial save game format info](
https://forums.civfanatics.com/threads/moo-save-file-layout.275055/).

Thanks to those who contributed code, ideas or bug reports.

Some code and documentation has been pilfered from [Chocolate Doom](https://github.com/chocolate-doom/chocolate-doom) and [VICE](https://vice-emu.sourceforge.io/).
