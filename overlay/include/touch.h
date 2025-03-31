#ifndef TOUCH_H
#define TOUCH_H

bool GetCalibratedPoint(int *x, int *y);

void RequestSamplingTPData();

void ResetTPData();

#endif