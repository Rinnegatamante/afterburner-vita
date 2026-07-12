#include "android/AAssetManager.h"

#include <psp2/kernel/threadmgr.h>
#include <malloc.h>
#include <cstring>
#include <cstdio>
#include <string>
#include <fcntl.h>

#ifdef USE_SCELIBC_IO
#include <shim/libc_bridge.h>
#endif

#include "FalsoNDK_Utils.h"

typedef struct assetManager {
    int dummy = 0; // TODO: mb we will need to store something here in future
    SceKernelLwMutexWork mLock;
} assetManager;

typedef struct aAsset {
    char * filename;
    FILE* f;
    size_t bytesRead;
    size_t fileSize;
    bool opened = false;
} asset;

static AAssetManager * g_AAssetManager = nullptr;

AAssetManager * AAssetManager_create() {
    if (g_AAssetManager) return g_AAssetManager;

    auto * am = (assetManager *) malloc(sizeof(assetManager));
    am->dummy = 0;
    sceKernelCreateLwMutex(&am->mLock, "asset_mgr_lock", 0, 0, nullptr);

    g_AAssetManager = (AAssetManager *) am;
    return g_AAssetManager;
}

AAsset* AAssetManager_open(AAssetManager* mgr, const char* filename, int mode) {
#ifdef DATA_PATH
    std::string realp = std::string(DATA_PATH) + std::string("assets/") + std::string(filename);
#else
    std::string realp = std::string("app0:") + std::string("assets/") + std::string(filename);
#endif

    auto * a = new aAsset;
    a->filename = (char *) malloc(realp.length() + 1);
    strcpy(a->filename, realp.c_str());
    a->bytesRead = 0;

#ifdef USE_SCELIBC_IO
    a->f = sceLibcBridge_fopen((const char *)a->filename, "r");
#else
    a->f = fopen((const char *)a->filename, "r");
#endif

    if (!a->f) {
        free(a->filename);
        delete a;
        a = nullptr;
    } else {
#ifdef USE_SCELIBC_IO
        sceLibcBridge_fseek(a->f, 0, SEEK_END);
        a->fileSize = sceLibcBridge_ftell(a->f);
        sceLibcBridge_fseek(a->f, 0, SEEK_SET);
#else
        fseek(a->f, 0, SEEK_END);
        a->fileSize = ftell(a->f);
        fseek(a->f, 0, SEEK_SET);
#endif
        a->opened = true;
    }

    ALOGD("AAssetManager_open<%p>(%p, %s, %i): %p", __builtin_return_address(0), mgr, realp.c_str(), mode, a);

    return (AAsset *) a;
}

void AAsset_close(AAsset* asset) {
    ALOGD("AAsset_close<%p>(%p)", __builtin_return_address(0), asset);

    if (asset) {
        auto * a = (aAsset *) asset;
        free(a->filename);
        if (a->opened) {
#ifdef USE_SCELIBC_IO
            sceLibcBridge_fclose(a->f);
#else
            fclose(a->f);
#endif
        }
        delete a;
    }
}

int AAsset_read(AAsset* asset, void* buf, size_t count) {
    ALOGD("AAsset_read<%p>(%p, %p, %i)", __builtin_return_address(0), asset, buf, count);

    if (!asset) {
        return -1;
    }

    auto * a = (aAsset *) asset;

    if (!a->opened) {
        return -1;
    }

#ifdef USE_SCELIBC_IO
    size_t ret = sceLibcBridge_fread(buf, 1, count, a->f);
#else
    size_t ret = fread(buf, 1, count, a->f);
#endif

    if (ret > 0) {
        a->bytesRead += ret;
        return (int) ret;
    } else {
#ifdef USE_SCELIBC_IO
        if (sceLibcBridge_feof(a->f)) {
#else
        if (feof(a->f)) {
#endif
            return 0;
        } else {
            return -1;
        }
    }
}

off_t AAsset_seek(AAsset* asset, off_t offset, int whence) {
    ALOGD("AAsset_seek(%p, %d, %i)", asset, offset, whence);

    if (!asset) {
        return (off_t) -1;
    }

    auto * a = (aAsset *) asset;

    if (!a->opened) {
        return -1;
    }

#ifdef USE_SCELIBC_IO
    auto ret = (off_t) sceLibcBridge_fseek(a->f, offset, whence);
#else
    auto ret = (off_t) fseek(a->f, offset, whence);
#endif

    if (ret != 0) {
        return -1;
    }

#ifdef USE_SCELIBC_IO
    return (off_t) sceLibcBridge_ftell(a->f);
#else
    return (off_t) ftell(a->f);
#endif
}

off_t AAsset_getRemainingLength(AAsset* asset) {
    ALOGD("AAsset_getRemainingLength");
    if (!asset) {
        return (off_t) -1;
    }

    auto * a = (aAsset *) asset;

    if (!a->opened) {
        return -1;
    }

#ifdef USE_SCELIBC_IO
    return (off_t)(a->fileSize - sceLibcBridge_ftell(a->f));
#else
    return (off_t)(a->fileSize - ftell(a->f));
#endif
}

off_t AAsset_getLength(AAsset* asset) {
    ALOGD("AAsset_getLength");
    if (!asset) {
        return (off_t) -1;
    }

    auto * a = (aAsset *) asset;

    return (off_t)a->fileSize;
}

AAssetDir* AAssetManager_openDir(AAssetManager* mgr, const char* dirName) {
    ALOGE("UNIMPLEMENTED: AAssetManager_openDir: %s", dirName);
    return (AAssetDir *)strdup("dummy");
}

const char* AAssetDir_getNextFileName(AAssetDir* assetDir) {
    ALOGE("UNIMPLEMENTED: AAssetDir_getNextFileName: %p", assetDir);
    return "";
}

void AAssetDir_close(AAssetDir* assetDir) {
    ALOGE("UNIMPLEMENTED: AAssetDir_close");
    free(assetDir);
}

int AAsset_openFileDescriptor(AAsset* asset, off_t* outStart, off_t* outLength) {
    if (!asset) {
        ALOGW("AAsset_openFileDescriptor(%p, %p, %p): asset is null", asset, outStart, outLength);
        return -1;
    }
    auto * a = (aAsset *) asset;
    if (outStart) *outStart = 0;
    if (outLength) *outLength = a->fileSize;
    if (a->opened) {
#ifdef USE_SCELIBC_IO
        sceLibcBridge_fclose(a->f);
#else
        fclose(a->f);
#endif
        a->f = NULL;
        a->opened = false;
    }
    int ret = open(a->filename, O_RDONLY);
    ALOGD("AAsset_openFileDescriptor(%p/\"%s\", %p, %p): ret %i", asset, a->filename, outStart, outLength, ret);
    return ret;
}
