#pragma once

#include "config.h"
#include "IO/GPIO.h"

#define BATTERY_SENSE_R1 10000 // 10k
#define BATTERY_SENSE_R2 1000  // 1k

extern float batteryGetVoltage();
extern float batteryGetVoltageSmooth();
extern int batteryGetPercentage();
extern int batteryGetPercentageSmooth();
extern void batteryUpdate();