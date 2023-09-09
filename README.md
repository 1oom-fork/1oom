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
Ctrl-[] to adjust sfx volume.
Ctrl-Shift-[] to adjust music volume.
Otherwise it controls like the original.


Abbreviations
=============

The following abbreviations appear in the documentation and code:

- MOO1: Master of Orion 1 as represented by the v1.3 DOS binaries
- OSG: The Official Strategy Guide (ISBN 1-55958-507-2)


Acknowledgements
================

Original game by Simtex Software, published 1993 by MicroProse.
Thanks for the great game and fuckings for not releasing the source code.

Thanks to Something Awful forum user Thotimx for the MOO1 LP which was a
major inspiration in starting this project. Unfortunately the goon writes
parenthesis without the leading space(such as here) like a retard, making the
Le'ts Play [sic] a painful read.

Ideas and text snippets have been taken from kyrub's unofficial patch 1.40m
Readme. Thanks for the patch.

Thanks to
http://www.shikadi.net/wiki/modding/index.php?title=XMI_Format&oldid=6874
for the music format info.

Thanks to CivFanatics forum user sargon0 for partial save game format info in
http://forums.civfanatics.com/threads/moo-save-file-layout.275055/

Thanks to those who contributed code, ideas or bug reports.

Some code has been pilfered from Chocolate Doom and VICE.

HACKING and PHILOSOPHY are based on Chocolate Doom.
