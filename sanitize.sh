autoreconf -fi
./configure --enable-fault-finding
make clean
make -j4
cd src
UBSAN_OPTIONS="print_stacktrace=1" ./1oom_classic_sdl2 -uiscale YOMAMA
cd ..

