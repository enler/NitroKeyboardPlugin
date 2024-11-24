#include <nds/ndstypes.h>
#include "fs.h"

#ifndef OVERLAY_ID
#define OVERLAY_ID -1
#endif

extern void (*Orig_OverlayStaticInitBegin[])();
extern void (*Orig_OverlayStaticInitEnd[])();

void LoadOverlay();

void (*const OverlayStaticInitFunc)() = LoadOverlay;

void LoadOverlay() {
    static bool loaded = false;
    if (loaded) return;
    FS_LoadOverlay(0, OVERLAY_ID);
    loaded = true;
    if (Orig_OverlayStaticInitBegin && Orig_OverlayStaticInitEnd) {
        for (void (**func)() = Orig_OverlayStaticInitBegin; func < Orig_OverlayStaticInitEnd; func++) {
            if (*func)
                (*func)();
        }
    }
}