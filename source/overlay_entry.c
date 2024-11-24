#include <nds/ndstypes.h>
#include <gl2d.h>
#include "keyboard.h"

void OverlayInit() {
    StartKeyboardMonitorThread();
}