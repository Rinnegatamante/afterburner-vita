#include "linux/fndk_pipe.h"

#include <psp2/kernel/threadmgr.h>
#include <cerrno>

#include "FalsoNDK_Utils.h"

#define PIPEFD_MARGIN 384
#define PIPEFD_MAX_COUNT 32
#define PIPEFD_MAX_FD (PIPEFD_MAX_COUNT * 2)

#define MSGPIPE_MEMTYPE_USER_MAIN 0x40
#define MSGPIPE_THREAD_ATTR_PRIO (0x8 | 0x4)

#ifndef SCE_KERNEL_MSG_PIPE_MODE_FULL
#define SCE_KERNEL_MSG_PIPE_MODE_FULL 0x00000001U
#endif

typedef struct pipefd_internal {
    int readfd = -1; // >=0 indicates that it's in use
    int writefd = -1;
    int msgpipe = -1;
    bool readable{};
    bool writeable{};
} pipefd_internal;

static pipefd_internal pipefd_pool[PIPEFD_MAX_COUNT];
static SceKernelLwMutexWork pipefd_pool_mutex{};

__attribute__((constructor))
static void pipefd_pool_init() {
    sceKernelCreateLwMutex(&pipefd_pool_mutex, "pipefd_pool_mutex", 0, 0, nullptr);
}

int fndk_pipe(int pipefd[2]) {
#ifdef DEBUG_PIPEFD
    ALOGD("fndk_pipe: called\n");
#endif

    sceKernelLockLwMutex(&pipefd_pool_mutex, 1, nullptr);

    int ret = sceKernelCreateMsgPipe("fndk_pipe", MSGPIPE_MEMTYPE_USER_MAIN, MSGPIPE_THREAD_ATTR_PRIO, 4 * 4096, NULL);
    if (ret < 0) {
        #ifdef DEBUG_PIPEFD
            ALOGD("fndk_pipe: sceKernelCreateMsgPipe failed\n");
        #endif
        sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
        return -1;
    }

    pipefd_internal * pipe = nullptr;
    for (int i = 0, u = 0; u < PIPEFD_MAX_FD; i++, u+=2) {
        if (pipefd_pool[i].readfd == -1) {
            pipe = &pipefd_pool[i];

            pipe->readfd = u + PIPEFD_MARGIN;
            pipe->writefd = u + 1 + PIPEFD_MARGIN;
            pipe->readable = false;
            pipe->writeable = true;
            pipe->msgpipe = ret;

            break;
        }
    }

    if (!pipe) {
        sceKernelDeleteMsgPipe(ret);
        sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
        errno = EMFILE;
        return -1;
    }

    pipefd[0] = pipe->readfd;
    pipefd[1] = pipe->writefd;

#ifdef DEBUG_PIPEFD
    ALOGD("fndk_pipe: pipe<%i, %i> initialized", pipe->readfd, pipe->writefd);
#endif

    sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
    return 0;
}

ssize_t fndk_pipe_read(int fd, void *buf, size_t count) {
    if (fd < PIPEFD_MARGIN || fd >= PIPEFD_MARGIN + PIPEFD_MAX_FD) {
        errno = EBADF;
        return -1;
    }

    int idx = (fd - PIPEFD_MARGIN) / 2;
    sceKernelLockLwMutex(&pipefd_pool_mutex, 1, NULL);

    pipefd_internal * pipe = &pipefd_pool[idx];
    if (pipe->readfd != fd) {
        sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
        errno = EBADF;
        return -1;
    }

    int msgpipe = pipe->msgpipe;
    ssize_t rlen = count;
    if (rlen > 4 * 4096) rlen = 4 * 4096;
    sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);

#ifdef DEBUG_PIPEFD
    ALOGD("fndk_pipe_read: reading from pipe fd %i", fd);
#endif

    size_t pResult;
    ssize_t ret = sceKernelReceiveMsgPipe(msgpipe, buf, rlen, 1, &pResult, NULL);

    sceKernelLockLwMutex(&pipefd_pool_mutex, 1, NULL);
    if (ret != 0 || pipefd_pool[idx].readfd != fd) {
        sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
        errno = (ret != 0) ? EIO : EBADF;
        return -1;
    }

    ret = rlen;
    pipe->writeable = true;
    if (pResult == 0) {
#ifdef DEBUG_PIPEFD
        ALOGD("fndk_pipe_read: pipe fd %i set as NOT readable", fd);
#endif
        pipe->readable = false;
    }

#ifdef DEBUG_PIPEFD
    ALOGD("fndk_pipe_read: pipe fd %i, count %i, ret %i", fd, count, ret);
#endif
    sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
    return ret;
}

ssize_t fndk_pipe_write(int fd, const void *buf, size_t count) {
    if (fd < PIPEFD_MARGIN || fd >= PIPEFD_MARGIN + PIPEFD_MAX_FD) {
        errno = EBADF;
        return -1;
    }

    int idx = (fd - PIPEFD_MARGIN) / 2;
    sceKernelLockLwMutex(&pipefd_pool_mutex, 1, NULL);

    pipefd_internal * pipe = &pipefd_pool[idx];
    if (pipe->writefd != fd) {
        sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
        errno = EBADF;
        return -1;
    }

    int msgpipe = pipe->msgpipe;
    size_t len = count;
    if (len > 4 * 4096) len = 4 * 4096;
    sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);

#ifdef DEBUG_PIPEFD
    ALOGD("fndk_pipe_write: writing to pipe fd %i", fd);
#endif

    ssize_t ret = sceKernelSendMsgPipe(msgpipe, (void *)buf, len, SCE_KERNEL_MSG_PIPE_MODE_FULL, NULL, NULL);

    sceKernelLockLwMutex(&pipefd_pool_mutex, 1, NULL);
    if (ret != 0 || pipefd_pool[idx].writefd != fd) {
        sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
        errno = (ret != 0) ? EIO : EBADF;
        return -1;
    }

    ret = len;
    pipe->readable = true;

#ifdef DEBUG_PIPEFD
    ALOGD("fndk_pipe_write: pipe fd %i set as readable, count %i, ret %i", fd, count, ret);
#endif

    sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
    return ret;
}

void fndk_pipe_status(int fd, bool * is_readable, bool * is_writeable, bool consume) {
    if (fd < PIPEFD_MARGIN || fd >= PIPEFD_MARGIN + PIPEFD_MAX_FD) {
        *is_readable = false;
        *is_writeable = false;
        return;
    }

    int idx = (fd - PIPEFD_MARGIN) / 2;
    sceKernelLockLwMutex(&pipefd_pool_mutex, 1, NULL);

    pipefd_internal * pipe = &pipefd_pool[idx];
    if (pipe->readfd != fd && pipe->writefd != fd) {
        *is_readable = false;
        *is_writeable = false;
        sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
        return;
    }

    *is_readable = pipe->readable;
    *is_writeable = pipe->writeable;
    if (consume) {
        pipe->readable = false;
        pipe->writeable = false;
    }

    sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
}

int fndk_pipe_close(int fd) {
    if (fd < PIPEFD_MARGIN || fd >= PIPEFD_MARGIN + PIPEFD_MAX_FD) {
        errno = EBADF;
        return -1;
    }

    int idx = (fd - PIPEFD_MARGIN) / 2;

    sceKernelLockLwMutex(&pipefd_pool_mutex, 1, nullptr);

    pipefd_internal * pipe = &pipefd_pool[idx];
    if (pipe->readfd != fd && pipe->writefd != fd) {
        sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
        errno = EBADF;
        return -1;
    }

    sceKernelDeleteMsgPipe(pipe->msgpipe);
    pipe->readfd = -1;
    pipe->writefd = -1;
    pipe->msgpipe = -1;
    pipe->readable = false;
    pipe->writeable = false;

    sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
    return 0;
}

bool is_pipe(int fd) {
#ifdef FNDK_SAFER_SLOWER
    if (fd < PIPEFD_MARGIN || fd >= PIPEFD_MARGIN + PIPEFD_MAX_FD) return false;
    int idx = (fd - PIPEFD_MARGIN) / 2;
    sceKernelLockLwMutex(&pipefd_pool_mutex, 1, NULL);
    bool result = pipefd_pool[idx].readfd == fd || pipefd_pool[idx].writefd == fd;
    sceKernelUnlockLwMutex(&pipefd_pool_mutex, 1);
    return result;
#else
    return (fd >= PIPEFD_MARGIN) && (fd < (PIPEFD_MARGIN + PIPEFD_MAX_FD));
#endif
}
