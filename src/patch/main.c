#include "patch_1_3a.h"


int main(int argc, char **argv)
{
    const bin_patch_status_t status = get_patch_set_status(patch_1_3a);
    if (status == PATCH_STATUS_INVALID) {
        printf("Wrong file\n");
    } else if (status == PATCH_STATUS_PATCHED) {
        remove_patch_set(patch_1_3a);
        printf("Patch removed\n");
    } else if (status == PATCH_STATUS_ORIGINAL) {
        apply_patch_set(patch_1_3a);
        printf("Patch applied\n");
    }
    getchar();
    return 0;
}
