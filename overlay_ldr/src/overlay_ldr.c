#include <nds/ndstypes.h>
#include "nitro/fs.h"

#ifndef OVERLAY_ID
#define OVERLAY_ID -1
#endif

#define NITROCODE_BEGIN 0xDEC00621
#define NITROCODE_END   0x2106C0DE

extern volatile const u32 nitrocode[];
extern void (*Orig_OverlayStaticInitBegin[])();
extern void (*Orig_OverlayStaticInitEnd[])();

void LoadOverlay();

void (*const OverlayStaticInitFunc)() = LoadOverlay;

static bool IsOverlayLoaded() {
    return nitrocode[0] == NITROCODE_BEGIN && nitrocode[1] == NITROCODE_END;
}

void LoadOverlay() {
    if (!IsOverlayLoaded()) {
        FS_LoadOverlay(0, OVERLAY_ID);
    }
    if (Orig_OverlayStaticInitBegin && Orig_OverlayStaticInitEnd) {
        for (void (**func)() = Orig_OverlayStaticInitBegin; func < Orig_OverlayStaticInitEnd; func++) {
            if (*func)
                (*func)();
        }
    }
}
