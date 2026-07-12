/*
 * shim/fndk_sensors.cpp
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 * Copyright (C) 2026 Ellie J Turner
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file for details.
 */

#include "shim/fndk_sensors.h"

#include <psp2/motion.h>
#include <psp2/kernel/threadmgr.h>

#include "FalsoNDK_Utils.h"

ASensorEventQueue * sensorEventQueue;

void fndk_sensors_init(ASensorEventQueue * queue) {
    sceMotionStartSampling(); // Enable sensors

    sensorEventQueue = queue;

    SceUID t = sceKernelCreateThread("fndk_sensors_poll", fndk_sensors_poll, 64, 32*1024, 0, 0, nullptr);
    sceKernelStartThread(t, 0, nullptr);
}

int fndk_sensors_poll(SceSize args, void * argp) {
    while (true) {
        ASensor* sensors[ASENSOR_COUNT_MAX];
        ASensorEventQueue_getEnabledSensors(sensorEventQueue, sensors);

        SceMotionSensorState sensor;
        sceMotionGetSensorState(&sensor, 1);

        for (auto * s : sensors) {
            if (!s) break;

            switch (ASensor_getType(s)) {
            case ASENSOR_TYPE_ACCELEROMETER: {
                ASensorVector v;
                v.y = sensor.accelerometer.x * ASENSOR_STANDARD_GRAVITY * -1;
                v.z = sensor.accelerometer.y * ASENSOR_STANDARD_GRAVITY * -1;
                v.x = sensor.accelerometer.z * ASENSOR_STANDARD_GRAVITY * -1;

                ASensorEvent e;
                e.version = sizeof(ASensorEvent);
                e.type = ASENSOR_TYPE_ACCELEROMETER;
                e.sensor = ASensor_getHandle(s);
                e.acceleration = v;

                ASensorEventQueue_enqueueEvent(sensorEventQueue, &e);
                break;
            }
            default: {
                ALOGE("sensors: not implemented sensor %s", ASensor_getStringType(s));
                break;
            }
            }
        }

        sceKernelDelayThread(15000);
    }
}
