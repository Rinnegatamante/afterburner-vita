/*
 * shim/fndk_controls.cpp
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 * Copyright (C) 2026 Ellie J Turner
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file for details.
 */

#include "shim/fndk_controls.h"

#include <psp2/kernel/threadmgr.h>
#include <cstdio>
#include <cstring>

#include "android/keycodes.h"
#include "android/AInput.h"

extern "C" {
    float L_INNER_DEADZONE __attribute__((weak)) = 0.16f;
    float R_INNER_DEADZONE __attribute__((weak)) = 0.16f;
    int fndk_button_event_source __attribute__((weak)) = AINPUT_SOURCE_GAMEPAD;
}

#define BACK_TOUCH_MARGIN 100

AInputQueue * inputQueue;
SceTouchPanelInfo panelInfoBack;

void pollTouch();
void pollPad();

void coord_normalize(float * x, float * y, float deadzone) {
    float magnitude = sqrtf((*x * *x) + (*y * *y));
    if (magnitude < deadzone) {
        *x = 0;
        *y = 0;
        return;
    }

    // normalize
    *x = *x / magnitude;
    *y = *y / magnitude;

    float clamped = magnitude > 1.0f ? 1.0f : magnitude;
    float multiplier = ((clamped - deadzone) / (1 - deadzone));
    *x = *x * multiplier;
    *y = *y * multiplier;
}

void fndk_controls_init(AInputQueue * queue) {
    // Enable analog sticks and touchscreen
    sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);

    sceTouchGetPanelInfo(SCE_TOUCH_PORT_BACK, &panelInfoBack);

    inputQueue = queue;

    SceUID t = sceKernelCreateThread("fndk_controls_poll", fndk_controls_poll, 64, 32*1024, 0, 0, nullptr);
    sceKernelStartThread(t, 0, nullptr);
}

int fndk_controls_poll(SceSize args, void * argp) {
    while (true) {
        pollPad();
        pollTouch();
        sceKernelDelayThread(16666);
    }
}

SceTouchData touch_old;
SceTouchData touch;
inputEvent ev;
int numPointersDown = 0;

int getIdxById(inputEvent * e, int id) {
    for (int i = 0; i < e->motion_ptrcount; ++i) {
        if (e->motion_ptridx[i] == id) {
            return i;
        }
    }
    return -1;
}

void removeById(inputEvent * e, int id) {
    int idx = getIdxById(e, id);

    inputEvent ev_backup = *e;
    memset(e->motion_ptridx, 0, sizeof(e->motion_ptridx));
    memset(e->motion_x, 0, sizeof(e->motion_x));
    memset(e->motion_y, 0, sizeof(e->motion_y));
    e->motion_ptrcount--;

    int u = 0;
    for (int i = 0; i < ev_backup.motion_ptrcount; ++i) {
        if (i == idx) continue;
        e->motion_ptridx[u] = ev_backup.motion_ptridx[i];
        e->motion_x[u] = ev_backup.motion_x[i];
        e->motion_y[u] = ev_backup.motion_y[i];
        u++;
    }
}

// Vita touch ids just keep incrementing from 0 to 127 and wrap but Android
// reuses the lowest free id and games might index arrays by it. So map every
// contact to a small "slot" and report that as the pointer id instead.
#define MAX_POINTERS 10
static int slot_hw_id[MAX_POINTERS] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static int slotByHwId(int hw_id) {
    for (int i = 0; i < MAX_POINTERS; ++i) {
        if (slot_hw_id[i] == hw_id) return i;
    }
    return -1;
}

static int slotAlloc(int hw_id) {
    for (int i = 0; i < MAX_POINTERS; ++i) {
        if (slot_hw_id[i] == -1) {
            slot_hw_id[i] = hw_id;
            return i;
        }
    }
    return -1;
}

void pollTouch() {
    memcpy(&touch_old, &touch, sizeof(touch_old));

    int numPointersMoved = 0;

    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
    if (touch.reportNum > 0) {
        for (int i = 0; i < touch.reportNum; i++) {
            int finger_down = 0;

            if (touch_old.reportNum > 0) {
                for (int j = 0; j < touch_old.reportNum; j++) {
                    if (touch.report[i].id == touch_old.report[j].id) {
                        finger_down = 1;
                    }
                }
            }

            float x = ((float)touch.report[i].x * 960.f / 1920.0f);
            float y = ((float)touch.report[i].y * 544.f / 1088.0f);

            // Send touch down event only if finger wasn't already down before.
            // slotAlloc failing also keeps numPointersDown within ev's arrays.
            if (!finger_down) {
                int finger_id = slotAlloc(touch.report[i].id);
                if (finger_id == -1) continue;

                ev.source = AINPUT_SOURCE_TOUCHSCREEN;
                ev.motion_ptrcount = numPointersDown + 1;
                ev.motion_x[numPointersDown] = x;
                ev.motion_y[numPointersDown] = y;
                ev.motion_ptridx[numPointersDown] = finger_id;
                ev.type = AINPUT_EVENT_TYPE_MOTION;

                // Get global event state to have up-to-date indices and coordinates,
                // but send a copy to not send MOVE too early / too often
                inputEvent ev_ptrdown = ev;
                if (numPointersDown == 0) {
                    ev_ptrdown.motion_action = AMOTION_EVENT_ACTION_DOWN;
                } else {
                    // For Pointer* actions, we have to set pointer index in respective bits
                    ev_ptrdown.motion_action = AMOTION_EVENT_ACTION_POINTER_DOWN | (numPointersDown << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                }

                numPointersDown++;
                AInputEvent* aie = AInputEvent_create(&ev_ptrdown);
                AInputQueue_enqueueEvent(inputQueue, aie);
            }
            // Otherwise, send touch move, but only if it actually moved,
            // a held finger shouldn't spam MOVE every poll
            else {
                int finger_id = slotByHwId(touch.report[i].id);
                int idx = (finger_id == -1) ? -1 : getIdxById(&ev, finger_id);
                if (idx != -1 && (ev.motion_x[idx] != x || ev.motion_y[idx] != y)) {
                    ev.motion_x[idx] = x;
                    ev.motion_y[idx] = y;
                    numPointersMoved++;
                }
            }
        }
    }

    if (numPointersMoved > 0) {
        ev.motion_action = AMOTION_EVENT_ACTION_MOVE;
        ev.type = AINPUT_EVENT_TYPE_MOTION;

        AInputEvent* aie = AInputEvent_create(&ev);
        AInputQueue_enqueueEvent(inputQueue, aie);
    }

    // some fingers might have been let go
    if (touch_old.reportNum > 0) {
        for (int i = 0; i < touch_old.reportNum; i++) {
            int finger_up = 1;
            if (touch.reportNum > 0) {
                for (int j = 0; j < touch.reportNum; j++) {
                    if (touch.report[j].id == touch_old.report[i].id) {
                        finger_up = 0;
                    }
                }
            }

            if (finger_up == 1) {
                float x = ((float)touch_old.report[i].x * 960.f / 1920.0f);
                float y = ((float)touch_old.report[i].y * 544.f / 1088.0f);

                int finger_id = slotByHwId(touch_old.report[i].id);
                if (finger_id == -1) continue;
                slot_hw_id[finger_id] = -1;

                int idx = getIdxById(&ev, finger_id);
                if (idx != -1) {
                    ev.motion_x[idx] = x;
                    ev.motion_y[idx] = y;

                    if (numPointersDown == 1) {
                        ev.motion_action = AMOTION_EVENT_ACTION_UP;
                    } else {
                        // For Pointer* actions, we have to set pointer index in respective bits
                        ev.motion_action = AMOTION_EVENT_ACTION_POINTER_UP | (idx << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                    }

                    numPointersDown--;

                    AInputEvent* aie = AInputEvent_create(&ev);
                    AInputQueue_enqueueEvent(inputQueue, aie);

                    removeById(&ev, finger_id);
                }
            }
        }
    }

    if (touch.reportNum == 0) {
        // nothing is touching the screen => drop any stale state left from a missed lift
        if (numPointersDown != 0 || ev.motion_ptrcount != 0) {
            numPointersDown = 0;
            memset(&ev, 0, sizeof(ev));
        }
        for (int i = 0; i < MAX_POINTERS; ++i)
            slot_hw_id[i] = -1;
    }
}

extern "C" {
    ButtonMapping fndk_button_mapping[] __attribute__((weak)) = {
            { SCE_CTRL_UP,        AKEYCODE_DPAD_UP },
            { SCE_CTRL_DOWN,      AKEYCODE_DPAD_DOWN },
            { SCE_CTRL_LEFT,      AKEYCODE_DPAD_LEFT },
            { SCE_CTRL_RIGHT,     AKEYCODE_DPAD_RIGHT },
            { SCE_CTRL_CROSS,     AKEYCODE_BUTTON_A },
            { SCE_CTRL_CIRCLE,    AKEYCODE_BUTTON_B },
            { SCE_CTRL_SQUARE,    AKEYCODE_BUTTON_X },
            { SCE_CTRL_TRIANGLE,  AKEYCODE_BUTTON_Y },
            { SCE_CTRL_L1,        AKEYCODE_BUTTON_L1 },
            { SCE_CTRL_R1,        AKEYCODE_BUTTON_R1 },
            { SCE_CTRL_L2,        AKEYCODE_BUTTON_L2 },
            { SCE_CTRL_R2,        AKEYCODE_BUTTON_R2 },
            { SCE_CTRL_START,     AKEYCODE_BUTTON_START },
            { SCE_CTRL_SELECT,    AKEYCODE_BUTTON_SELECT },
    };
    int fndk_button_mapping_count __attribute__((weak)) = 14;
}

uint32_t old_buttons = 0, current_buttons = 0, pressed_buttons = 0, released_buttons = 0;
float lx = 0, ly = 0, rx = 0, ry = 0;

inputEvent stickInputEvent;
int sticksDown = 0;
float x_old = 0.0f, y_old = 0.0f, z_old = 0.0f, rz_old = 0.0f, hat_x_old = 0.0f, hat_y_old = 0.0f;
bool ltPressed_old = false, rtPressed_old = false;

void sendJoyEvent(float x, float y, float z, float rz, float hat_x, float hat_y, bool ltPressed, bool rtPressed) {
    if (x != x_old || y != y_old || z != z_old || rz != rz_old
        || hat_x != hat_x_old || hat_y != hat_y_old
        || ltPressed != ltPressed_old || rtPressed != rtPressed_old)
    {
        stickInputEvent.source = AINPUT_SOURCE_JOYSTICK;
        stickInputEvent.motion_ptrcount = sticksDown + 1;
        stickInputEvent.motion_x[0] = x;
        stickInputEvent.motion_y[0] = y;
        stickInputEvent.motion_z[0] = z;
        stickInputEvent.motion_rz[0] = rz;
        stickInputEvent.motion_hat_x[0] = hat_x;
        stickInputEvent.motion_hat_y[0] = hat_y;
        stickInputEvent.motion_lt[0] = ltPressed ? 1.0 : 0.0;
        stickInputEvent.motion_rt[0] = rtPressed ? 1.0 : 0.0;
        stickInputEvent.motion_ptridx[0] = 0;
        stickInputEvent.type = AINPUT_EVENT_TYPE_MOTION;

        stickInputEvent.motion_action = AMOTION_EVENT_ACTION_MOVE;
        AInputEvent* aie = AInputEvent_create(&stickInputEvent);
        AInputQueue_enqueueEvent(inputQueue, aie);

        x_old = x;
        y_old = y;
        z_old = z;
        rz_old = rz;
        hat_x_old = hat_x;
        hat_y_old = hat_y;
        ltPressed_old = ltPressed;
        rtPressed_old = rtPressed;
    }
}

void pollPad() {
    SceCtrlData pad;
    sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

    old_buttons = current_buttons;
    current_buttons = pad.buttons;

    if (!(current_buttons & SCE_CTRL_L2  || current_buttons & SCE_CTRL_R2)) {
        SceTouchData t;
        sceTouchPeek(SCE_TOUCH_PORT_BACK, &t, 1);

        for (int i = 0; i < t.reportNum; i++) {
            if (t.report[i].y < (panelInfoBack.minAaY + panelInfoBack.maxAaY) / 2) {
                if (t.report[i].x < (panelInfoBack.minAaX + panelInfoBack.maxAaX) / 2) {
                    if (t.report[i].x >= BACK_TOUCH_MARGIN) current_buttons |= SCE_CTRL_L2;
                } else {
                    if (t.report[i].x < (panelInfoBack.maxAaX - BACK_TOUCH_MARGIN)) current_buttons |= SCE_CTRL_R2;
                }
            }
        }
    }

    pressed_buttons = current_buttons & ~old_buttons;
    released_buttons = ~current_buttons & old_buttons;

    for (int i = 0; i < fndk_button_mapping_count; i++) {
        ButtonMapping & m = fndk_button_mapping[i];
        if (pressed_buttons & m.sce_button) {
            inputEvent e;
            e.source = fndk_button_event_source; // Override fndk_button_event_source to change, e.g. to AINPUT_SOURCE_DPAD
            e.keycode = m.android_button;
            e.action = AKEY_EVENT_ACTION_DOWN;
            e.type = AINPUT_EVENT_TYPE_KEY;

            AInputEvent* aie = AInputEvent_create(&e);
            AInputQueue_enqueueEvent(inputQueue, aie);
        } else if (released_buttons & m.sce_button) {
            inputEvent e;
            e.source = fndk_button_event_source;
            e.keycode = m.android_button;
            e.action = AKEY_EVENT_ACTION_UP;
            e.type = AINPUT_EVENT_TYPE_KEY;

            AInputEvent *aie = AInputEvent_create(&e);
            AInputQueue_enqueueEvent(inputQueue, aie);
        }
    }

    lx = ((float)pad.lx - 128.0f) / 128.0f;
    ly = ((float)pad.ly - 128.0f) / 128.0f;
    rx = ((float)pad.rx - 128.0f) / 128.0f;
    ry = ((float)pad.ry - 128.0f) / 128.0f;

    coord_normalize(&lx, &ly, L_INNER_DEADZONE);
    coord_normalize(&rx, &ry, R_INNER_DEADZONE);

    sendJoyEvent(lx, ly, rx, ry, 0, 0, current_buttons & SCE_CTRL_L2, current_buttons & SCE_CTRL_R2);
}
