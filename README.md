1oom
====

1oom is a Master of Orion (1993) game engine recreation.
1oom is Free Software (GPLv2), see COPYING.


1 Installation
===============

1oom requires a copy of the Master of Orion (v1.3) LBX files.

1.1 Windows, MSDOS
------------------

Simply copy the EXE and DLLs (if any) to your MOO1 directory.
Alternatively copy your MOO1 LBX files to your 1oom directory.
See doc/usage_common.txt if you prefer to use a separate directory.

1.2 Unix (Linux)
----------------

1oom requires the following libraries:

- SDL (libsdl1.2 or libsdl2):
http://www.libsdl.org
- SDL_mixer (libsdl-mixer1.2 or libsdl2-mixer):
http://www.libsdl.org/projects/SDL_mixer/

Also recommended:

- libsamplerate:
http://www.mega-nerd.com/libsamplerate/

Check your distribution's package manager or the library
website on how to install them.


2 Configuration
===============

See doc/usage_common.txt.


3 Development
=============

See INSTALL and HACKING for more information.


4 Executables
=============

1oom consists of a few executables:

- 1oom_classic_*    (the 1993 UI)
- 1oom_cmdline      (proof of concept textual UI)
- 1oom_lbxview_*    (for viewing LBX files)
- 1oom_lbxedit      (for editing LBX files)
- 1oom_pbxmake      (for creating PBX files)
- 1oom_pbxdump      (for dumping PBX files)
- 1oom_gfxconv      (for converting GFX for use in PBX files)
- 1oom_saveconv     (for converting save game files)
- 1oom_*_sdl1       (using SDL 1.2.x)
- 1oom_*_sdl2       (using SDL 2.x)
- 1oom_*_alleg4     (using Allegro 4.x)

See doc/usage_* for more documentation.

Some ports have shorter filenames due to filesystem limitations.


5 Abbreviations
===============

The following abbreviations appear in the documentation and code:

- MOO1: Master of Orion 1 as represented by the v1.3 DOS binaries
- OSG: The Official Strategy Guide (ISBN 1-55958-507-2)


6 Acknowledgements
==================

Most of the credit for this software belongs to the progammer who authored
1oom v1.0 under the pseudonym Kilgore Trout Mask Replicant. Thank you for
creating this and publishing it free and open source!

The original game Master of Orion was developed  by Simtex Software and
published 1993 by MicroProse. Thanks for the great game!

Thanks to Alan Emrich and Tom Hughes for documenting the game mechanics and AI
decision making in great detail in the official strategy guide.

Ideas and text snippets have been taken from kyrub's unofficial patch 1.40m
Readme. Thanks for the patch!

Thanks to [shikadi.net](http://www.shikadi.net) for documenting the
[music format](http://www.shikadi.net/wiki/modding/index.php?title=XMI_Format&oldid=6874).

Thanks to CivFanatics forum user sargon0 for
[partial save game format info](http://forums.civfanatics.com/threads/moo-save-file-layout.275055/).

Thanks to those who contributed code, ideas or bug reports.

Some code has been pilfered from Chocolate Doom and VICE.

The files HACKING and PHILOSOPHY are based on Chocolate Doom.
