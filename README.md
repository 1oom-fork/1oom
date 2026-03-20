1oom
====

1oom aims to accurately reproduce the original DOS version of Master of Orion Classic (1993)
in a form that can be run on modern computers. Direct use of AI-generated code is prohibited in 1oom.
1oom is Free Software (GPLv2), see [COPYING](COPYING).


Tools
=====

- lbxview (for viewing LBX files)
- saveconv (for converting save game files)


Installation
============

To use this software, you must legally own an original copy of Master of Orion (v1.3).
1oom requires a copy of the Master of Orion (v1.3) LBX files.

Windows
-------

Simply copy the EXE and DLLs (if any) to your MOO1 directory.

Unix (Linux)
------------

1oom requires the following libraries:

- [SDL](https://www.libsdl.org) (libsdl1.2)
- [SDL_mixer](https://www.libsdl.org/projects/SDL_mixer/) (libsdl-mixer1.2)

Also recommended:

- [libsamplerate](https://www.mega-nerd.com/libsamplerate/)

Check your distribution's package manager or the library
website on how to install them.


Configuration
=============

See [CONFIGURATION](CONFIGURATION).


Development
===========

See [INSTALL](INSTALL) and [HACKING](HACKING) for more information.
Also: [How to build 1oom on Windows](https://kilgoretroutmaskreplicant.gitlab.io/plain-html/howto_build_windows.html).


Controls
========

- Middle click or Ctrl-F10 to grab/ungrab input.
- Ctrl-Esc to quit (without saving).
- Ctrl-[] to adjust sfx volume.
- Ctrl-Shift-[] to adjust music volume.

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

Thanks to MyName aka Duzh87_54MSU for restoring 1oom from its broken state
and for creating [1oom v2.0 (vanilla)](https://sourcecraft.dev/fork1oom/1oom).

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
