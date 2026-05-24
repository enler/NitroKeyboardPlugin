#include <nds/ndstypes.h>
#include "nitro/fs.h"

#ifndef OVERLAY_ID
#define OVERLAY_ID -1
#endif

void LoadOverlay() {
    static int counter;
    static bool loaded = false;
    if (loaded)
        return;
    if (!counter++)
        return;
    FS_LoadOverlay(0, OVERLAY_ID);
    loaded = true;
}
