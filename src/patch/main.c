#include "patch_1_3a.h"


int main(int argc, char **argv)
{
    execute_patcher("STARMAP.EXE", patch_1_3a);
    return 0;
}
