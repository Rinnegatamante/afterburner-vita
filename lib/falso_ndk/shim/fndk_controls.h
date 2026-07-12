/*
 * reimpl/controls.h
 *
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
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

void fndk_controls_init(AInputQueue * queue);
int fndk_controls_poll(SceSize args, void * argp);

#ifdef __cplusplus
};
#endif
