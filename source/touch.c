#include <nds/ndstypes.h>
#include "touch.h"

extern u8 HW_TOUCHPANEL_BUF[];

struct SPITpData {
    u32     x:12;
    u32     y:12;
    u32     touch:1;
    u32     validity:2;
    u32     dummy:5;
};

bool GetCalibratedPoint(int *x, int *y) {
    struct SPITpData data;
    TPData tpData;

    *(u8*)&data = HW_TOUCHPANEL_BUF[0];
    *((u8*)&data + 1) = HW_TOUCHPANEL_BUF[1];
    *((u8*)&data + 2) = HW_TOUCHPANEL_BUF[2];
    *((u8*)&data + 3) = HW_TOUCHPANEL_BUF[3];

    tpData.x = data.x;
    tpData.y = data.y;
    tpData.touch = data.touch;
    tpData.validity = data.validity;
    TP_GetCalibratedPoint(&tpData, &tpData);
    if (tpData.touch)
    {
        *x = tpData.x;
        *y = tpData.y;
        return true;
    }
    else {
        return false;
    }
}