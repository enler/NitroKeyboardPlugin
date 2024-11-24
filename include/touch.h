#ifndef TOUCH_H
#define TOUCH_H

typedef struct {
    u16     x;
    u16     y;
    u16     touch;
    u16     validity;
}
TPData;

void TP_GetCalibratedPoint(TPData *disp, const TPData *raw);

bool GetCalibratedPoint(int *x, int *y);

void RequestSamplingTPData();

void ResetTPData();

#endif