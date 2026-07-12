/*
 * shim/fndk_sensors.h
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 * Copyright (C) 2026 Ellie J Turner
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file for details.
 */

#pragma once

#include <psp2/kernel/threadmgr.h>
#include "android/ASensor.h"

void fndk_sensors_init(ASensorEventQueue * queue);
int fndk_sensors_poll(SceSize args, void * argp);
