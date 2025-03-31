#include <nds/ndstypes.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <calico/arm/common.h>
#include "nitro/tp.h"
#include "touch.h"

extern u8 HW_TOUCHPANEL_BUF[];

struct SPITpData {
    u32 x:12;
    u32 y:12;
    u32 touch:1;
    u32 validity:2;
    u32 dummy:5;
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

void RequestSamplingTPData() {
    if (REG_IPC_FIFO_CR & IPC_FIFO_ERROR) {
        REG_IPC_FIFO_CR |= (IPC_FIFO_ENABLE | IPC_FIFO_ERROR);
        return;
    }
    ArmIrqState state = armIrqLockByPsr();
    if (REG_IPC_FIFO_CR & IPC_FIFO_SEND_FULL) {
        armIrqUnlockByPsr(state);
        return;
    }
    REG_IPC_FIFO_TX = 0xC0000006;
    armIrqUnlockByPsr(state);
}

void ResetTPData() {
    struct SPITpData data = {0};
    data.validity = 3;
    HW_TOUCHPANEL_BUF[0] = *(u8*)&data;
    HW_TOUCHPANEL_BUF[1] = *((u8*)&data + 1);
    HW_TOUCHPANEL_BUF[2] = *((u8*)&data + 2);
    HW_TOUCHPANEL_BUF[3] = *((u8*)&data + 3);
}