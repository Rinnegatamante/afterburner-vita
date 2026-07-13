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

so_hook joy_update;
void (*Joypad_Finalize)(uint32_t *this);
void (*Joypad_Destructor)(uint32_t *this);
void (*Joypad_Constructor)(uint32_t *this);
void (*Joypad_Initialize)(uint32_t *this, int unk);

void Joypad_Update(uint32_t *this, int unk) {
	if (!this[248]) {
		sceClibPrintf("Resetting broken Joypad struct\n");
		Joypad_Finalize(this);
		Joypad_Destructor(this);
		Joypad_Constructor(this);
		Joypad_Initialize(this, 0);
		Joypad_Update(this, unk);
		return;
	}
	
	SO_CONTINUE(int, joy_update, this, unk);
}

void so_patch(void) {
	sceKernelCreateLwMutex(&fmod_mutex, "FMOD mutex", 0, 0, NULL);
	
	// Skip age consent checks
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension17haveRemoveAgeGateEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension12isEnoughtAgeEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension17getGDPRFormFinishEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension18haveGDPRFormFinishEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension16isConsentCountryEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension15haveUserConsentEv"), ret1);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension16checkConsentGDPREv"), ret1);

	// Remove ads banner removal
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension27Android_stateUserRemovedAdsEv"), ret1);
	
	// Nuke some unused functions
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension10trackEventENSt6__ndk112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEE"), ret0);
	hook_addr(so_symbol(&so_mod, "_ZN12F2FExtension22Android_trackEventJSONERKNSt6__ndk112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEES8_"), ret0);	

	// Disable touch inputs rendering
	hook_addr(so_symbol(&so_mod, "_ZN24ABC_INPUT_VIRTUAL_BUTTON13RenderAButtonEv"), ret0);
	hook_addr(so_symbol(&so_mod, "_ZN24ABC_INPUT_VIRTUAL_BUTTON13RenderBButtonEv"), ret0);
	uintptr_t x_button_addr = (uintptr_t)so_symbol(&so_mod, "_ZN24ABC_INPUT_VIRTUAL_BUTTON13RenderXButtonEv") + 0x105;
	*(uint32_t *)x_button_addr = 0xBF00BF00; // nop, nop
	
	// For some reason the INPUT_JOYPAD struct gets corrupted sometimes when doing touch presses, so we detect when this happen and reset the struct into a sane state
	joy_update = hook_addr(so_symbol(&so_mod, "_ZN12INPUT_JOYPAD6UpdateERK14PRIMITIVE_TIME"), Joypad_Update);
	Joypad_Finalize = so_symbol(&so_mod, "_ZN12INPUT_JOYPAD8FinalizeEv");
	Joypad_Destructor = so_symbol(&so_mod, "_ZN12INPUT_JOYPADD2Ev");
	Joypad_Constructor = so_symbol(&so_mod, "_ZN12INPUT_JOYPADC2Ev");
	Joypad_Initialize =  so_symbol(&so_mod, "_ZN12INPUT_JOYPAD10InitializeEi");
	
	// Required for flagging for gamepad connection in checkGameController JNI method
	deviceAdded = (void *)so_symbol(&so_mod, "Java_com_sega_afterburnerclimax_AfterBurnerClimax_deviceAdded");
}
