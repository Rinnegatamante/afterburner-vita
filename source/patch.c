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

void *dummy_alloc() { return malloc(0x20); }

int (*deviceAdded)();

extern SceKernelLwMutexWork fmod_mutex;

void so_patch(void) {
	sceKernelCreateLwMutex(&fmod_mutex, "FMOD mutex", 0, 0, NULL);
	
	// Prevent crashes on NULL objects references
	hook_addr(so_symbol(&so_mod, "_ZN37PLATFORM_JAVA_NATIVE_ACTIVITY_MANAGER12GetJavaClassERK18PRIMITIVE_SUB_TEXT"), dummy_alloc);
	
	// Skip age consent checks
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension17haveRemoveAgeGateEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension12isEnoughtAgeEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension17getGDPRFormFinishEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension18haveGDPRFormFinishEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension16isConsentCountryEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension15haveUserConsentEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension16checkConsentGDPREv"), ret1);
	
	// Nuke some unused functions
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension10trackEventENSt6__ndk112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEE"), ret0);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension22Android_trackEventJSONERKNSt6__ndk112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEES8_"), ret0);	

	// Disable touch inputs rendering
	hook_addr(so_symbol(&so_mod, "_ZN24ABC_INPUT_VIRTUAL_BUTTON13RenderAButtonEv"), ret0);
	hook_addr(so_symbol(&so_mod, "_ZN24ABC_INPUT_VIRTUAL_BUTTON13RenderBButtonEv"), ret0);
	uintptr_t x_button_addr = (uintptr_t)so_symbol(&so_mod, "_ZN24ABC_INPUT_VIRTUAL_BUTTON13RenderXButtonEv") + 0x105;
	*(uint32_t *)x_button_addr = 0xBF00BF00; // nop, nop
	
	// Required for flagging for gamepad connection in checkGameController JNI method
	deviceAdded = (void *)so_symbol(&so_mod, "Java_com_sega_afterburnerclimax_AfterBurnerClimax_deviceAdded");
}
