/*
 * patch.c
 *
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "patch.h"

#include <kubridge.h>
#include <so_util/so_util.h>
#include <stdio.h>
#include <vitasdk.h>

#ifdef __cplusplus
extern "C" {
#endif
extern so_module so_mod;
#ifdef __cplusplus
};
#endif

#include "utils/logger.h"
#include <stdbool.h>

int ret0() { return 0; }
int ret1() { return 1; }

int (*deviceAdded)();

extern SceKernelLwMutexWork fmod_mutex;

void so_patch(void) {
	sceKernelCreateLwMutex(&fmod_mutex, "FMOD mutex", 0, 0, NULL);
	
	// Skip age consent checks
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension17haveRemoveAgeGateEv"), (uintptr_t)ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension12isEnoughtAgeEv"), (uintptr_t)ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension17getGDPRFormFinishEv"), (uintptr_t)ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension18haveGDPRFormFinishEv"), (uintptr_t)ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension16isConsentCountryEv"), (uintptr_t)ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension15haveUserConsentEv"), (uintptr_t)ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension16checkConsentGDPREv"), (uintptr_t)ret1);
	
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension24Android_getInternetStateEv"), (uintptr_t)ret1);

	// Remove ads banner removal
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension27Android_stateUserRemovedAdsEv"), (uintptr_t)ret1);
	
	// Nuke some unused functions
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension10trackEventENSt6__ndk112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEE"), (uintptr_t)ret0);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension22Android_trackEventJSONERKNSt6__ndk112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEES8_"), (uintptr_t)ret0);

	// Disable touch inputs rendering
	hook_addr(so_symbol(&so_mod, "_ZN24ABC_INPUT_VIRTUAL_BUTTON13RenderAButtonEv"), (uintptr_t)ret0);
	hook_addr(so_symbol(&so_mod, "_ZN24ABC_INPUT_VIRTUAL_BUTTON13RenderBButtonEv"), (uintptr_t)ret0);
	uintptr_t x_button_addr = (uintptr_t)so_symbol(&so_mod, "_ZN24ABC_INPUT_VIRTUAL_BUTTON13RenderXButtonEv") + 0x105;
	*(uint32_t *)x_button_addr = 0xBF00BF00; // nop, nop
	
	// Required for flagging for gamepad connection in checkGameController JNI method
	deviceAdded = (void *)so_symbol(&so_mod, "Java_com_sega_afterburnerclimax_AfterBurnerClimax_deviceAdded");
}
