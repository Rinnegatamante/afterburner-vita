#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI_Logger.h>
#include <falso_jni/FalsoJNI.h>
#include <string.h>
#include <vitasdk.h>
#include <vitaGL.h>
#include <stdlib.h>

// System-wide constant that's often used to determine Android version
// https://developer.android.com/reference/android/os/Build.VERSION.html#SDK_INT
// Possible values: https://developer.android.com/reference/android/os/Build.VERSION_CODES
const int SDK_INT = 19; // Android 4.4 / KitKat

/*
 * JNI Methods
*/

NameToMethodID nameToMethodId[] = {
	{ 1, "GetDataArchiveFilePath", METHOD_TYPE_OBJECT },
	{ 2, "GetApplicationDirectory", METHOD_TYPE_OBJECT }, 
	{ 3, "getLocalPath", METHOD_TYPE_OBJECT }, 
	{ 4, "getBundlePath", METHOD_TYPE_OBJECT }, 
	{ 5, "getAssertPath", METHOD_TYPE_OBJECT }, 
	{ 6, "getApplicationDirectory", METHOD_TYPE_OBJECT }, 
	{ 7, "getCocos2dxWritablePath", METHOD_TYPE_OBJECT }, 
	{ 8, "getLanguageCode", METHOD_TYPE_OBJECT }, 
	{ 9, "getDeviceDisplayLanguage", METHOD_TYPE_OBJECT }, 
	{ 10, "getDeviceLanguage", METHOD_TYPE_OBJECT }, 
	{ 11, "getRegionCode", METHOD_TYPE_OBJECT }, 
	{ 12, "getCountryCode", METHOD_TYPE_OBJECT }, 
	{ 13, "getValueDataString", METHOD_TYPE_OBJECT }, 
	{ 14, "getDeviceID", METHOD_TYPE_OBJECT },
	{ 15, "getPriceStringIAP", METHOD_TYPE_OBJECT },
	{ 16, "getApkPath", METHOD_TYPE_OBJECT },
	{ 17, "isTablet", METHOD_TYPE_BOOLEAN },
	{ 18, "HasIntroVideoFinished", METHOD_TYPE_BOOLEAN },
	{ 19, "checkInit", METHOD_TYPE_BOOLEAN },
	{ 20, "supportsLowLatency", METHOD_TYPE_BOOLEAN },
	{ 21, "getValueDataInt", METHOD_TYPE_INT },
	{ 22, "getSaveAge", METHOD_TYPE_INT },
	{ 23, "getInternetState", METHOD_TYPE_INT },
	{ 24, "getRotation", METHOD_TYPE_INT },
	{ 25, "getOutputSampleRate", METHOD_TYPE_INT },
	{ 26, "getOutputBlockSize", METHOD_TYPE_INT },
	{ 27, "getValueDataLong", METHOD_TYPE_LONG },
	{ 28, "getValueDataFloat", METHOD_TYPE_FLOAT },
	{ 29, "saveValueInt", METHOD_TYPE_VOID },
	{ 30, "saveValueBool", METHOD_TYPE_VOID },
	{ 31, "saveValueLong", METHOD_TYPE_VOID },
	{ 32, "saveValueFloat", METHOD_TYPE_VOID },
	{ 33, "saveValueString", METHOD_TYPE_VOID },
	{ 34, "callBackGDPRState", METHOD_TYPE_VOID },
	{ 35, "playIntroVideo", METHOD_TYPE_VOID },
	{ 36, "showMessageBox", METHOD_TYPE_VOID },
	{ 39, "getValueDataBool", METHOD_TYPE_BOOLEAN },
	{ 40, "checkGameController", METHOD_TYPE_VOID },
};

#define normalize_path(x) \
	char *s = strstr(x, "/"); \
	while (s) { \
		s[0] = '_'; \
		s = strstr(s, "/"); \
	}

void saveValueInt(jmethodID id, va_list args) {
	jstring _key = va_arg(args, jstring);
	const char *key = jni->GetStringUTFChars(&jni, _key, NULL);
	jint val = va_arg(args, jint);
	char fname[256];
	normalize_path(key)
	sprintf(fname, "ux0:data/afterburner/i_%s.bin", key);
	jni->ReleaseStringUTFChars(&jni, _key, key);
	FILE *f = fopen(fname, "wb");
	fwrite(&val, 1, sizeof(val), f);
	fclose(f);
}

void saveValueBool(jmethodID id, va_list args) {
	jstring _key = va_arg(args, jstring);
	const char *key = jni->GetStringUTFChars(&jni, _key, NULL);
	jboolean val = va_arg(args, jint);
	char fname[256];
	normalize_path(key)
	sprintf(fname, "ux0:data/afterburner/b_%s.bin", key);
	jni->ReleaseStringUTFChars(&jni, _key, key);
	FILE *f = fopen(fname, "wb");
	fwrite(&val, 1, sizeof(val), f);
	fclose(f);
}

void saveValueLong(jmethodID id, va_list args) {
	jstring _key = va_arg(args, jstring);
	const char *key = jni->GetStringUTFChars(&jni, _key, NULL);
	jlong val = va_arg(args, jlong);
	char fname[256];
	normalize_path(key)
	sprintf(fname, "ux0:data/afterburner/l_%s.bin", key);
	jni->ReleaseStringUTFChars(&jni, _key, key);
	FILE *f = fopen(fname, "wb");
	fwrite(&val, 1, sizeof(val), f);
	fclose(f);
}

void saveValueFloat(jmethodID id, va_list args) {
	jstring _key = va_arg(args, jstring);
	const char *key = jni->GetStringUTFChars(&jni, _key, NULL);
	jfloat val = va_arg(args, jfloat);
	char fname[256];
	normalize_path(key)
	sprintf(fname, "ux0:data/afterburner/f_%s.bin", key);
	jni->ReleaseStringUTFChars(&jni, _key, key);
	FILE *f = fopen(fname, "wb");
	fwrite(&val, 1, sizeof(val), f);
	fclose(f);
}

void saveValueString(jmethodID id, va_list args) {
	jstring _key = va_arg(args, jstring);
	const char *key = jni->GetStringUTFChars(&jni, _key, NULL);
	jstring _val = va_arg(args, jstring);
	const char *val = jni->GetStringUTFChars(&jni, _val, NULL);
	char fname[256];
	normalize_path(key)
	sprintf(fname, "ux0:data/afterburner/s_%s.bin", key);
	jni->ReleaseStringUTFChars(&jni, _key, key);
	FILE *f = fopen(fname, "wb");
	fwrite(val, 1, strlen(val), f);
	fclose(f);
	jni->ReleaseStringUTFChars(&jni, _val, val);
}

jboolean getValueDataBool(jmethodID id, va_list args) {
	jstring _key = va_arg(args, jstring);
	const char *key = jni->GetStringUTFChars(&jni, _key, NULL);
	jboolean val = va_arg(args, jint);
	char fname[256];
	normalize_path(key)
	sprintf(fname, "ux0:data/afterburner/b_%s.bin", key);
	FILE *f = fopen(fname, "rb");
	if (f) {
		jni->ReleaseStringUTFChars(&jni, _key, key);
		jboolean ret;
		fread(&ret, 1, sizeof(ret), f);
		fclose(f);
		return ret;
	}
	if (strstr(key, "age") || strstr(key, "consent") || strstr(key, "gdpr") || strstr(key, "coppa") || strstr(key, "legal"))
		val = JNI_TRUE;
	jni->ReleaseStringUTFChars(&jni, _key, key);
	return val;
}

jint getValueDataInt(jmethodID id, va_list args) {
	jstring _key = va_arg(args, jstring);
	const char *key = jni->GetStringUTFChars(&jni, _key, NULL);
	jint val = va_arg(args, jint);
	char fname[256];
	normalize_path(key)
	sprintf(fname, "ux0:data/afterburner/i_%s.bin", key);
	FILE *f = fopen(fname, "rb");
	if (f) {
		jni->ReleaseStringUTFChars(&jni, _key, key);
		jint ret;
		fread(&ret, 1, sizeof(ret), f);
		fclose(f);
		return ret;
	}
	if (strstr(key, "age") || strstr(key, "consent") || strstr(key, "gdpr") || strstr(key, "coppa") || strstr(key, "legal"))
		val = 1;
	jni->ReleaseStringUTFChars(&jni, _key, key);
	return val;
}

jlong getValueDataLong(jmethodID id, va_list args) {
	jstring _key = va_arg(args, jstring);
	const char *key = jni->GetStringUTFChars(&jni, _key, NULL);
	jlong val = va_arg(args, jlong);
	char fname[256];
	normalize_path(key)
	sprintf(fname, "ux0:data/afterburner/l_%s.bin", key);
	jni->ReleaseStringUTFChars(&jni, _key, key);
	FILE *f = fopen(fname, "rb");
	if (f) {
		jlong ret;
		fread(&ret, 1, sizeof(ret), f);
		fclose(f);
		return ret;
	}
	return val;
}

jfloat getValueDataFloat(jmethodID id, va_list args) {
	jstring _key = va_arg(args, jstring);
	const char *key = jni->GetStringUTFChars(&jni, _key, NULL);
	jfloat val = va_arg(args, jfloat);
	char fname[256];
	normalize_path(key)
	sprintf(fname, "ux0:data/afterburner/f_%s.bin", key);
	jni->ReleaseStringUTFChars(&jni, _key, key);
	FILE *f = fopen(fname, "rb");
	if (f) {
		jfloat ret;
		fread(&ret, 1, sizeof(ret), f);
		fclose(f);
		return ret;
	}
	return val;
}

jstring getValueDataString(jmethodID id, va_list args) {
	jstring _key = va_arg(args, jstring);
	const char *key = jni->GetStringUTFChars(&jni, _key, NULL);
	jstring val = va_arg(args, jstring);
	char fname[256];
	normalize_path(key)
	sprintf(fname, "ux0:data/afterburner/s_%s.bin", key);
	jni->ReleaseStringUTFChars(&jni, _key, key);
	FILE *f = fopen(fname, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		size_t sz = ftell(f);
		fseek(f, 0, SEEK_SET);
		char *arr = malloc(sz + 1);
		fread(arr, 1, sz, f);
		arr[sz] = 0;
		fclose(f);
		jstring ret = jni->NewStringUTF(&jni, arr);
		free(arr);
		return (jstring)ret;
	}
	return val;
}

void dummy(jmethodID id, va_list args) {
	fjni_log_info("dummy");
}

void showMessageBox(jmethodID id, va_list args) {
	jstring msg = va_arg(args, jstring);
	const char *cmsg = jni->GetStringUTFChars(&jni, msg, NULL);
	sceClibPrintf("showMessageBox %s\n", cmsg);
	jni->ReleaseStringUTFChars(&jni, msg, cmsg);
}

extern int (*deviceAdded)();
void checkGameController(jmethodID id, va_list args) {
	sceClibPrintf("Adding gamepad\n");
	deviceAdded();
}

jint getSaveAge(jmethodID id, va_list args) {
	return 3;
}

jint retZero(jmethodID id, va_list args) {
	return 0;
}

jint getOutputSampleRate(jmethodID id, va_list args) {
	return 48000;
}

jint getOutputBlockSize(jmethodID id, va_list args) {
	return 512;
}

jboolean HasIntroVideoFinished(jmethodID id, va_list args) {
    return JNI_TRUE;
}

jboolean retTrue(jmethodID id, va_list args) {
    return JNI_TRUE;
}

jboolean retFalse(jmethodID id, va_list args) {
    return JNI_FALSE;
}

jobject emptyString(jmethodID id, va_list args) {
	jstring ret = jni->NewStringUTF(&jni, "");
    return (jobject)ret;
}

jobject getMainPath(jmethodID id, va_list args) {
	jstring ret = jni->NewStringUTF(&jni, "ux0:data/afterburner");
    return (jobject)ret;
}

jobject getObbPath(jmethodID id, va_list args) {
	jstring ret = jni->NewStringUTF(&jni, "ux0:data/afterburner/main.obb");
    return (jobject)ret;
}

jobject getMainPathWithSlash(jmethodID id, va_list args) {
	jstring ret = jni->NewStringUTF(&jni, "ux0:data/afterburner/");
    return (jobject)ret;
}

jobject getLanguage(jmethodID id, va_list args) {
	int lang = -1;
	jstring ret;
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &lang);
	switch (lang) {
		case SCE_SYSTEM_PARAM_LANG_ITALIAN:
			ret = jni->NewStringUTF(&jni, "it");
			break;		
		case SCE_SYSTEM_PARAM_LANG_FRENCH:
			ret = jni->NewStringUTF(&jni, "fr");
			break;
		case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR:
			ret = jni->NewStringUTF(&jni, "pt");
			break;
		case SCE_SYSTEM_PARAM_LANG_RUSSIAN:
			ret = jni->NewStringUTF(&jni, "ru");
			break;
		case SCE_SYSTEM_PARAM_LANG_SPANISH:
			ret = jni->NewStringUTF(&jni, "es");
			break;
		case SCE_SYSTEM_PARAM_LANG_GERMAN:
			ret = jni->NewStringUTF(&jni, "de");
			break;
		default:
			ret = jni->NewStringUTF(&jni, "en");
			break;
	}
    return (jobject)ret;
}

jobject getCountryCode(jmethodID id, va_list args) {
	int lang = -1;
	jstring ret;
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &lang);
	switch (lang) {
		case SCE_SYSTEM_PARAM_LANG_ITALIAN:
			ret = jni->NewStringUTF(&jni, "IT");
			break;		
		case SCE_SYSTEM_PARAM_LANG_FRENCH:
			ret = jni->NewStringUTF(&jni, "FR");
			break;
		case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR:
			ret = jni->NewStringUTF(&jni, "BR");
			break;
		case SCE_SYSTEM_PARAM_LANG_RUSSIAN:
			ret = jni->NewStringUTF(&jni, "RU");
			break;
		case SCE_SYSTEM_PARAM_LANG_SPANISH:
			ret = jni->NewStringUTF(&jni, "ES");
			break;
		case SCE_SYSTEM_PARAM_LANG_GERMAN:
			ret = jni->NewStringUTF(&jni, "DE");
			break;
		default:
			ret = jni->NewStringUTF(&jni, "US");
			break;
	}
    return (jobject)ret;
}

extern void video_open(const char *path);
extern uint8_t video_finished();
extern int draw_video_frame();
extern void video_close();
void playIntroVideo(jmethodID id, va_list args) {
	sceClibPrintf("Start intro video\n");
	SceIoStat st;
	if (sceIoGetstat("ux0:data/afterburner/sega_720.mp4", &st) < 0) {
		return;
	}
	video_open("ux0:data/afterburner/sega_720.mp4");
	while (!video_finished()) {
		if (draw_video_frame()) {
			SceCtrlData pad;
			sceCtrlPeekBufferPositive(0, &pad, 1);
			if (pad.buttons & SCE_CTRL_CROSS || pad.buttons & SCE_CTRL_START) {
				video_close();
			}
			vglSwapBuffers(GL_FALSE);
		}
	}
	SceKernelFreeMemorySizeInfo info;
	info.size = sizeof(SceKernelFreeMemorySizeInfo);
	sceKernelGetFreeMemorySize(&info);
	vglPhycontMemLazyInit(info.size_phycont);
}

MethodsBoolean methodsBoolean[] = {
	{ 17, retFalse }, // isTablet
	{ 18, HasIntroVideoFinished }, // HasIntroVideoFinished
	{ 19, retTrue }, // checkInit
	{ 20, retFalse }, // supportsLowLatency
	{ 39, getValueDataBool }, // getValueDataBool
};

MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {
	{ 28, getValueDataFloat },  // getValueDataFloat
};
MethodsLong methodsLong[] = {
	{ 27, getValueDataLong },  // getValueDataLong
};
MethodsShort methodsShort[] = {};

MethodsInt methodsInt[] = {
	{ 21, getValueDataInt },  // getValueDataInt
	{ 22, getSaveAge }, // getSaveAge
	{ 23, retZero }, // getInternetState
	{ 24, retZero }, // getRotation
	{ 25, getOutputSampleRate }, // getOutputSampleRate
	{ 26, getOutputBlockSize }, // getOutputBlockSize
};

extern void *dummy_alloc();

MethodsObject methodsObject[] = {
	{ 1, getObbPath }, // GetDataArchiveFilePath
	{ 2, getMainPath }, // GetApplicationDirectory
	{ 3, getMainPath }, // getLocalPath
	{ 4, getMainPath }, // getBundlePath
	{ 5, getMainPath }, // getAssertPath
	{ 6, getMainPath }, // getApplicationDirectory
	{ 7, getMainPathWithSlash }, // getCocos2dxWritablePath
	{ 8, getLanguage }, // getLanguageCode
	{ 9, getLanguage }, // getDeviceDisplayLanguage
	{ 10, getLanguage }, // getDeviceLanguage
	{ 11, getCountryCode }, // getRegionCode
	{ 12, getCountryCode }, // getCountryCode
	{ 13, getValueDataString }, // getValueDataString
	{ 14, emptyString }, // getDeviceID
	{ 15, emptyString }, // getPriceStringIAP
	{ 16, getMainPath }, // getApkPath
};

MethodsVoid methodsVoid[] = {
	{ 29, saveValueInt }, // saveValueInt
	{ 30, saveValueBool }, // saveValueBool
	{ 31, saveValueLong }, // saveValueLong
	{ 32, saveValueFloat }, // saveValueFloat
	{ 33, saveValueString }, // saveValueString
	{ 34, dummy }, // callBackGDPRState
	{ 35, playIntroVideo }, // playIntroVideo
	{ 36, showMessageBox }, // showMessageBox
	{ 40, checkGameController }, // checkGameController
};

/*
 * JNI Fields
*/

// System-wide constant that applications sometimes request
// https://developer.android.com/reference/android/content/Context.html#WINDOW_SERVICE
char WINDOW_SERVICE[] = "window";

NameToFieldID nameToFieldId[] = {
	{ 0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT }, 
	{ 1, "SDK_INT", FIELD_TYPE_INT },
};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {
	{ 1, SDK_INT },
};
FieldsObject fieldsObject[] = {
	{ 0, WINDOW_SERVICE },
};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
