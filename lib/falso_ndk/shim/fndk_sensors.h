#pragma once

#include <psp2/kernel/threadmgr.h>
#include "android/ASensor.h"

void fndk_sensors_init(ASensorEventQueue * queue);
int fndk_sensors_poll(SceSize args, void * argp);
