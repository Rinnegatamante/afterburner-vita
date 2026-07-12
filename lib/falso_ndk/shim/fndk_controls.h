/*
 * shim/fndk_controls.h
 *
 * Copyright (C) 2022 Volodymyr Atamanenko
 * Copyright (C) 2026 Ellie J Turner
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file for details.
 */

#pragma once

#include <psp2/kernel/threadmgr.h>
#include <psp2/touch.h>
#include <psp2/ctrl.h>
#include <psp2/motion.h>
#include <math.h>

#include "android/AInput.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t sce_button;
    int32_t android_button;
} ButtonMapping;

/* Override these symbols to use custom button bindings. */
extern ButtonMapping fndk_button_mapping[];
extern int fndk_button_mapping_count;

/* Override to change the AInputEvent source reported for button key events
 * (defaults to AINPUT_SOURCE_GAMEPAD). Some games expect AINPUT_SOURCE_KEYBOARD
 * and others. */
extern int fndk_button_event_source;

void fndk_controls_init(AInputQueue * queue);
int fndk_controls_poll(SceSize args, void * argp);

#ifdef __cplusplus
};
#endif
