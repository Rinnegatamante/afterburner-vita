#include "android/AInput.h"

#include <deque>
#include <vector>
#include <psp2/kernel/threadmgr.h>
#include <cstring>
#include <cerrno>

#include "FalsoNDK_Utils.h"
#include "linux/fndk_epoll.h"
#include "linux/fndk_eventfd.h"
#include "linux/fndk_unistd.h"
#include "shim/fndk_controls.h"

#define EVENT_POOL_SIZE 64
static inputEvent _event_pool_storage[EVENT_POOL_SIZE];
static inputEvent* _event_pool_free[EVENT_POOL_SIZE];
static int _event_pool_top = 0;
static SceKernelLwMutexWork _event_pool_lock;

__attribute__((constructor))
static void _event_pool_init() {
    sceKernelCreateLwMutex(&_event_pool_lock, "event_pool_lock", 0, 0, nullptr);
    for (int i = 0; i < EVENT_POOL_SIZE; ++i)
        _event_pool_free[i] = &_event_pool_storage[i];
    _event_pool_top = EVENT_POOL_SIZE;
}

static AInputQueue * g_AInputQueue = nullptr;

typedef struct inputQueue {
    int mDispatchFd;
    std::vector<ALooper*> mAppLoopers;
    SceKernelLwMutexWork mLock;
    std::deque<AInputEvent*> mPendingEvents;
} inputQueue;

AInputQueue * AInputQueue_create() {
    if (g_AInputQueue) return g_AInputQueue;

    auto * iq = new inputQueue();
    iq->mDispatchFd = fndk_eventfd(0, FNDK_EFD_NONBLOCK | FNDK_EFD_SEMAPHORE);

    if (iq->mDispatchFd < 0) {
        ALOGE("eventfd creation for AInputQueue failed: %s\n", strerror(errno));
    } else {
        ALOGD("Created eventfd for AInputQueue: #%i", iq->mDispatchFd);
    }

    sceKernelCreateLwMutex(&iq->mLock, "input_queue_lock", 0, 0, nullptr);
    g_AInputQueue = reinterpret_cast<AInputQueue *>(iq);

    fndk_controls_init(g_AInputQueue);

    return g_AInputQueue;
}

void AInputQueue_attachLooper(AInputQueue* queue, ALooper* looper,
                              int ident, ALooper_callbackFunc callback, void* data) {
    if (!queue || !looper) return;
    auto * q = reinterpret_cast<inputQueue *>(queue);

    sceKernelLockLwMutex(&q->mLock, 1, nullptr);

    for (size_t i = 0; i < q->mAppLoopers.size(); i++) {
        if (looper == q->mAppLoopers[i]) {
            sceKernelUnlockLwMutex(&q->mLock, 1);
            return;
        }
    }

    q->mAppLoopers.push_back(looper);
#if DEBUG_POLL_AND_WAKE
    ALOGD("AInputQueue (%p) : attaching looper (%p) to our FD %i", q, looper, q->mDispatchFd);
#endif
    ALooper_addFd(looper, q->mDispatchFd, ident, ALOOPER_EVENT_INPUT, callback, data);
    sceKernelUnlockLwMutex(&q->mLock, 1);
}

void AInputQueue_detachLooper(AInputQueue* queue) {
    if (!queue) return;
    auto * q = reinterpret_cast<inputQueue *>(queue);

    sceKernelLockLwMutex(&q->mLock, 1, nullptr);
    for (size_t i = 0; i < q->mAppLoopers.size(); i++) {
        ALooper_removeFd(q->mAppLoopers[i], q->mDispatchFd);
    }
    q->mAppLoopers.clear();
    sceKernelUnlockLwMutex(&q->mLock, 1);
}

int32_t AInputQueue_hasEvents(AInputQueue* queue) {
    if (!queue) {
        ALOGE("AInputQueue_hasEvents: bad queue");
        return -1;
    }

    auto * q = reinterpret_cast<inputQueue *>(queue);
    sceKernelLockLwMutex(&q->mLock, 1, nullptr);
    int32_t result = q->mPendingEvents.empty() ? 0 : 1;
    sceKernelUnlockLwMutex(&q->mLock, 1);
    return result;
}

int32_t AInputQueue_getEvent(AInputQueue* queue, AInputEvent** outEvent) {
    if (!queue) {
        ALOGE("AInputQueue_getEvent: bad queue");
        return -1;
    }

    if (!outEvent) {
        ALOGE("AInputQueue_getEvent: bad outEvent");
        return -1;
    }

    auto * q = reinterpret_cast<inputQueue *>(queue);

    sceKernelLockLwMutex(&q->mLock, 1, nullptr);
    *outEvent = NULL;
    if (!q->mPendingEvents.empty()) {
        *outEvent = q->mPendingEvents.front();
        q->mPendingEvents.pop_front();
    }

    if (q->mPendingEvents.empty()) {
        uint64_t byteread;
        ssize_t nRead;
        do {
            nRead = fndk_read(q->mDispatchFd, &byteread, sizeof(byteread));
            if (nRead < 0 && errno != EAGAIN) {
                ALOGW("Failed to read from native dispatch pipe: %s", strerror(errno));
            }
        } while (nRead == 8); // reduce eventfd semaphore to 0
    }

    int ret = *outEvent != NULL ? 0 : -EAGAIN;
    sceKernelUnlockLwMutex(&q->mLock, 1);
    return ret;
}

int32_t AInputQueue_preDispatchEvent(AInputQueue* queue, AInputEvent* event) {
    // Never pre-dispatch
    return false;
}

void AInputQueue_finishEvent(AInputQueue* queue, AInputEvent* event, int handled) {
    if (!event) return;
    auto* e = reinterpret_cast<inputEvent*>(event);
    if (e >= _event_pool_storage && e < _event_pool_storage + EVENT_POOL_SIZE) {
        sceKernelLockLwMutex(&_event_pool_lock, 1, nullptr);
        _event_pool_free[_event_pool_top++] = e;
        sceKernelUnlockLwMutex(&_event_pool_lock, 1);
    } else {
        free(event);
    }
}

void AInputQueue_enqueueEvent(AInputQueue* queue, AInputEvent* event) {
    if (!queue || !event) return;
    auto * q = reinterpret_cast<inputQueue *>(queue);

    sceKernelLockLwMutex(&q->mLock, 1, nullptr);
    q->mPendingEvents.push_back(event);
    if (q->mPendingEvents.size() == 1) {
        uint64_t payload = 1;
        int res = TEMP_FAILURE_RETRY(fndk_write(q->mDispatchFd, &payload, sizeof(payload)));
        if (res < 0 && errno != EAGAIN) {
            ALOGW("Failed writing to dispatch fd: %s", strerror(errno));
        }
    }
    sceKernelUnlockLwMutex(&q->mLock, 1);
}

/**
 * ========================
 */

AInputEvent *AInputEvent_create(const inputEvent *e) {
    sceKernelLockLwMutex(&_event_pool_lock, 1, nullptr);
    inputEvent* ret;
    if (_event_pool_top > 0) {
        ret = _event_pool_free[--_event_pool_top];
        sceKernelUnlockLwMutex(&_event_pool_lock, 1);
    } else {
        sceKernelUnlockLwMutex(&_event_pool_lock, 1);
        ret = static_cast<inputEvent*>(malloc(sizeof(inputEvent)));
    }
    memcpy(ret, e, sizeof(inputEvent));
    return reinterpret_cast<AInputEvent*>(ret);
}

int32_t AInputEvent_getType(const AInputEvent* event) {
    if (!event) return -1;
    auto * e = reinterpret_cast<const inputEvent *>(event);
    return e->type;
}

int32_t AInputEvent_getDeviceId(const AInputEvent* event) {
    if (!event) return -1;
    auto * e = reinterpret_cast<const inputEvent *>(event);
    return e->source; // in our case that's fine, hopefully. unless there are external pads connected?
}

int32_t AInputEvent_getSource(const AInputEvent* event) {
    if (!event) return AINPUT_SOURCE_UNKNOWN;
    auto * e = reinterpret_cast<const inputEvent *>(event);
    return e->source;
}

int32_t AKeyEvent_getAction(const AInputEvent* key_event) {
    if (!key_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(key_event);
    return e->action;
}

int32_t AKeyEvent_getKeyCode(const AInputEvent* key_event) {
    if (!key_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(key_event);
    return e->keycode;
}

int32_t AKeyEvent_getRepeatCount(const AInputEvent* key_event) {
    if (!key_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(key_event);
    return e->repeatcount;
}

int32_t AKeyEvent_getScanCode(const AInputEvent* key_event) {
    if (!key_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(key_event);
    return e->scancode;
}

int32_t AMotionEvent_getAction(const AInputEvent* motion_event) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    return e->motion_action;
}

size_t AMotionEvent_getPointerCount(const AInputEvent* motion_event) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    return e->motion_ptrcount;
}

int32_t AMotionEvent_getPointerId(const AInputEvent* motion_event, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    if (pointer_index >= 10) pointer_index = 9;
    return e->motion_ptridx[pointer_index];
}

float AMotionEvent_getX(const AInputEvent* motion_event, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    if (pointer_index >= 10) pointer_index = 9;
    return e->motion_x[pointer_index];
}

float AMotionEvent_getY(const AInputEvent* motion_event, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    if (pointer_index >= 10) pointer_index = 9;
    return e->motion_y[pointer_index];
}

float AMotionEvent_getAxisValue(const AInputEvent* motion_event,
int32_t axis, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = reinterpret_cast<const inputEvent *>(motion_event);
    if (pointer_index >= 10) pointer_index = 9;
    switch (axis) {
        case AMOTION_EVENT_AXIS_X:
            return e->motion_x[pointer_index];
        case AMOTION_EVENT_AXIS_Y:
            return e->motion_y[pointer_index];
        case AMOTION_EVENT_AXIS_Z:
            return e->motion_z[pointer_index];
        case AMOTION_EVENT_AXIS_RZ:
            return e->motion_rz[pointer_index];
        case AMOTION_EVENT_AXIS_HAT_X:
            return e->motion_hat_x[pointer_index];
        case AMOTION_EVENT_AXIS_HAT_Y:
            return e->motion_hat_y[pointer_index];
        case AMOTION_EVENT_AXIS_BRAKE:
        case AMOTION_EVENT_AXIS_LTRIGGER:
            return e->motion_lt[pointer_index];
        case AMOTION_EVENT_AXIS_GAS:
        case AMOTION_EVENT_AXIS_RTRIGGER:
            return e->motion_rt[pointer_index];
        case AMOTION_EVENT_AXIS_RX:
        case AMOTION_EVENT_AXIS_RY:
            // These are often requested for joysticks but not applicable to the Vita
            return 0;
        default:
            ALOGE("AMotionEvent_getAxisValue: unexpected axis %i", axis);
            return 0;
    }
}

float AMotionEvent_getHistoricalAxisValue(const AInputEvent* motion_event,
                                          int32_t axis, size_t pointer_index, size_t history_index) {
    // FIXME: Actual history impl here
    return AMotionEvent_getAxisValue(motion_event, axis, pointer_index);
}
