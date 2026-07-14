/*
 * main.c
 *
 * ARMv7 Shared Libraries loader. Soulcalibur Edition
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021-2023 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/init.h"
#include "utils/glutil.h"
#include "utils/settings.h"
#include "utils/logger.h"

#include <psp2/kernel/threadmgr.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>
#include "utils/dialog.h"
#include <string.h>
#include "utils/utils.h"

#include <falso_jni/FalsoJNI_ImplBridge.h>

#include <falso_ndk/FalsoNDK.h>
#include <vitasdk.h>
#include <stdio.h>

extern const int SDK_INT;

int _newlib_heap_size_user = 200 * 1024 * 1024;
int sceLibcHeapSize = 56 * 1024 * 1024;

so_module so_mod;

void *real_main(void *argv) {
	SceAppUtilInitParam appUtilParam;
	SceAppUtilBootParam appUtilBootParam;
	memset(&appUtilParam, 0, sizeof(SceAppUtilInitParam));
	memset(&appUtilBootParam, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&appUtilParam, &appUtilBootParam);
	
	soloader_init_all();

	void (*setObbFileName)(void *env, void *class, jstring path, int unk) = (void *) so_symbol(&so_mod, "Java_com_sega_f2fextension_F2FAndroidJNI_setObbFileName");
	void (*setApkPath)(void *env, void *class, jstring path) = (void *) so_symbol(&so_mod, "Java_com_sega_f2fextension_F2FAndroidJNI_nativeSetApkPath");
	void (*setScreenSize)(void *env, void *class, int w, int h) = (void *) so_symbol(&so_mod, "Java_com_sega_f2fextension_F2FAndroidJNI_setScreenSize");
	void (*setScreenScaleDensity)(void *env, void *class, float w, float h) = (void *) so_symbol(&so_mod, "Java_com_sega_f2fextension_F2FAndroidJNI_setScreenScaleDesity");
	int (*ANativeActivity_onCreate)(ANativeActivity *activity, void *savedState, size_t savedStateSize) = (void *) so_symbol(&so_mod, "ANativeActivity_onCreate");
	void (*JNI_OnLoad)(void *vm, void* unk) = (void *) so_symbol(&so_mod, "JNI_OnLoad");
	
	JNI_OnLoad(&jvm, NULL);
	
	jstring apk_path = jni->NewStringUTF(&jni, "ux0:data/afterburner/");
	sceClibPrintf("setApkPath %x\n", setApkPath);
	setApkPath(&jni, NULL, apk_path);
	
	sceClibPrintf("setObbFileName %x\n", setObbFileName);
	jstring obb_path = jni->NewStringUTF(&jni, "main.obb");
	setObbFileName(&jni, NULL, obb_path, 0);
	
	sceClibPrintf("setScreenSize %x\n", setScreenSize);
	setScreenSize(&jni, NULL, 960, 544);
	
	sceClibPrintf("setScreenScaleDensity %x\n", setScreenScaleDensity);
	setScreenScaleDensity(&jni, NULL, 1.0f, 1.0f);
	
	ANativeActivity *activity = malloc(sizeof(ANativeActivity));
	activity->callbacks       = malloc(sizeof(ANativeActivityCallbacks));
	activity->env             = &jni;
	activity->vm              = &jvm;
	activity->clazz           = (jclass)0x42424242;
	activity->internalDataPath = DATA_PATH "assets/";
	activity->externalDataPath = DATA_PATH "assets/";
	activity->sdkVersion      = SDK_INT;
	activity->instance        = NULL;
	log_info("Created NativeActivity object");

	ANativeActivity_onCreate(activity, NULL, 0);
	log_info("ANativeActivity_onCreate() passed");

	activity->callbacks->onStart(activity);
	log_info("onStart() passed");

	AInputQueue *aInputQueue = AInputQueue_create();
	activity->callbacks->onInputQueueCreated(activity, aInputQueue);
	log_info("onInputQueueCreated() passed");

	ANativeWindow *aNativeWindow = ANativeWindow_create();
	activity->callbacks->onNativeWindowCreated(activity, aNativeWindow);
	log_info("onNativeWindowCreated() passed");

	activity->callbacks->onWindowFocusChanged(activity, 1);
	log_info("onWindowFocusChanged() passed");

	log_info("Main thread shutting down");

	sceKernelExitDeleteThread(0);
	return NULL;
}

int main(int argc, char *argv[]) {	
	sceSysmoduleLoadModule(SCE_SYSMODULE_AVPLAYER);
	sceClibPrintf("Loading FMOD Studio...\n");
	if (!file_exists("ur0:/data/libfmodstudio.suprx"))
		fatal_error("Error libfmodstudio.suprx is not installed.");
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	int ret = sceNetShowNetstat();
	SceNetInitParam initparam;
	if (ret == SCE_NET_ERROR_ENOTINIT) {
		initparam.memory = malloc(141 * 1024);
		initparam.size = 141 * 1024;
		initparam.flags = 0;
		sceNetInit(&initparam);
	}
	sceClibPrintf("sceKernelLoadStartModule %x\n", sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, NULL, 0, NULL, NULL));
	sceClibPrintf("sceKernelLoadStartModule %x\n", sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, NULL, 0, NULL, NULL));
	sceClibPrintf("sceKernelLoadStartModule %x\n", sceKernelLoadStartModule("ur0:data/libfmodstudio.suprx", 0, NULL, 0, NULL, NULL));
	
	pthread_t t;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 0x400000);
	pthread_create(&t, &attr, real_main, NULL);

	return sceKernelExitDeleteThread(0);
}
