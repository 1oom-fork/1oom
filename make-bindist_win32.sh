#!/bin/sh

# based on make-bindist_win32.sh from VICE 3.1
# Written by
#  Marco van den Heuvel <blackystardust68@yahoo.com>

STRIP=$1
TOPSRCDIR=$2
ZIPKIND=$3

EXECUTABLES="1oom_classic_sdl1 1oom_classic_sdl2 1oom_cmdline 1oom_gfxconv 1oom_lbxview_sdl1 1oom_lbxview_sdl2 1oom_pbxmake 1oom_saveconv"

BINDISTDIR=bindisttemp

for i in $EXECUTABLES
do
    if [ ! -e src/$i.exe ]
    then
        echo Error: file $i not found, do a \"make\" first
        exit 1
    fi
done

if [ -e $TOPSRCDIR/.git ]
    then
    VERSIONSTR=`git describe --tags`
else
    VERSIONSTR=vUnknown
fi

PACKAGESTR=1oom-$VERSIONSTR-win32
ZIPNAME=$PACKAGESTR.zip

mkdir $BINDISTDIR

echo $PACKAGESTR > $BINDISTDIR/version.txt

for i in $EXECUTABLES
do
  $STRIP src/$i.exe
  cp src/$i.exe $BINDISTDIR
done

cp $TOPSRCDIR/doc/*.txt $BINDISTDIR
cp $TOPSRCDIR/README $BINDISTDIR/README.txt
cp $TOPSRCDIR/COPYING $BINDISTDIR/COPYING.txt

unix2dos -q $BINDISTDIR/*.txt

if [ -e extrabindist_common ]
    then
    cp extrabindist_common/* $BINDISTDIR
fi

if [ -e extrabindist_win32 ]; then
    cp extrabindist_win32/* $BINDISTDIR
fi

if test x"$ZIPKIND" = "xzip"; then
    zip -9 -j -q $ZIPNAME $BINDISTDIR/* || die
    echo zip $ZIPNAME created
    rm -f -r $BINDISTDIR
else
    echo dir $BINDISTDIR created
fi
