// SPDX-License-Identifier: Apache-2.0
#include <zaptronics/core.h>
#include <string.h>

int main(void)
{
    if (ZAPEE_VERSION_MAJOR != 0) {
        return 1;
    }
    if (strcmp(ZAPEE_VERSION_STRING, "0.2.0") != 0) {
        return 2;
    }
    return 0;
}
