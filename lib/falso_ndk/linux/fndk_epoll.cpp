#include "linux/fndk_epoll.h"

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <map>
#include <sys/unistd.h>
#include <psp2/kernel/threadmgr.h>

#include "FalsoNDK_Utils.h"
#include "linux/fndk_eventfd.h"
#include "linux/fndk_pipe.h"

#define EPOLL_FD_MARGIN 128
#define EPOLL_FD_MAX 64

typedef struct epollElement {
    int fd;
    fndk_epoll_event e;
} epollElement;

typedef struct _epoll_fd_internal {
    int fd = -1; // >=0 indicates that it's in use
    // `interest` is heap-allocated for a reason. Static std::map behaves weird on the Vita.
    std::map<int, epollElement> * interest = nullptr;
} _epoll_fd_internal;

static _epoll_fd_internal epoll_fd_pool[EPOLL_FD_MAX];
static SceKernelLwMutexWork _epoll_lock;

__attribute__((constructor))
static void _epoll_global_init() {
    sceKernelCreateLwMutex(&_epoll_lock, "epoll_lock", 0, 0, NULL);
    for (int i = 0; i < EPOLL_FD_MAX; ++i) {
        epoll_fd_pool[i].fd = -1;
        epoll_fd_pool[i].interest = nullptr;
    }
}

void _lock() {
    sceKernelLockLwMutex(&_epoll_lock, 1, NULL);
}

void _unlock() {
    sceKernelUnlockLwMutex(&_epoll_lock, 1);
}

int fndk_epoll_create(int size) {
    if (size <= 0) {
        errno = EINVAL;
        return -1;
    }

    return fndk_epoll_create1(0);
}

int fndk_epoll_create1(int flags) {
    // flags can be ditched since the only flag is O_CLOEXEC and we never exec()?
    if (flags != 0 && flags != FNDK_EPOLL_CLOEXEC) {
        errno = EINVAL;
        return -1;
    }

    _lock();

    _epoll_fd_internal * fd = nullptr;
    for (int i = 0; i < EPOLL_FD_MAX; ++i) {
        if (epoll_fd_pool[i].fd == -1) {
            epoll_fd_pool[i].fd = i + EPOLL_FD_MARGIN;
            epoll_fd_pool[i].interest = new std::map<int, epollElement>;
            fd = &epoll_fd_pool[i];
            break;
        }
    }

    if (!fd) {
        _unlock();
        errno = EMFILE;
        return -1;
    }

    _unlock();
    return fd->fd;
}

bool is_epoll(int fd) {
    return (fd >= EPOLL_FD_MARGIN) && (fd < (EPOLL_FD_MARGIN + EPOLL_FD_MAX));
}

int fndk_epoll_close(int epfd) {
    if (epfd < EPOLL_FD_MARGIN || epfd >= EPOLL_FD_MARGIN + EPOLL_FD_MAX) {
        errno = EBADF;
        return -1;
    }

    _lock();
    _epoll_fd_internal * epoll = &epoll_fd_pool[epfd - EPOLL_FD_MARGIN];
    if (epoll->fd != epfd) {
        _unlock();
        errno = EBADF;
        return -1;
    }

    delete epoll->interest;
    epoll->interest = nullptr;
    epoll->fd = -1;
    _unlock();
    return 0;
}

#ifdef DEBUG_EPOLL
const char * __op_to_str(int op) {
    switch (op) {
        case FNDK_EPOLL_CTL_ADD:
            return "FNDK_EPOLL_CTL_ADD";
        case FNDK_EPOLL_CTL_DEL:
            return "FNDK_EPOLL_CTL_DEL";
        case FNDK_EPOLL_CTL_MOD:
            return "FNDK_EPOLL_CTL_MOD";
    }
    return "FNDK_EPOLL_CTL_UNKNOWN";
}
#endif

int fndk_epoll_ctl(int epfd, int op, int fd, struct fndk_epoll_event *event) {
    if (epfd < EPOLL_FD_MARGIN || epfd >= EPOLL_FD_MARGIN + EPOLL_FD_MAX || fd < 0) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_ctl(epfd:%i, op:%s, fd:%i): EBADF: epfd or fd is not a valid file descriptor.", epfd, __op_to_str(op), fd);
#endif
        errno = EBADF;
        return -1;
    }

    _lock();

    _epoll_fd_internal * epoll = &epoll_fd_pool[epfd - EPOLL_FD_MARGIN];

    if (epoll->fd != epfd || fd == epfd) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_ctl(epfd:%i, op:%s, fd:%i): EINVAL: epfd is not an epoll file descriptor, or fd is the same as epfd.", epfd, __op_to_str(op), fd);
#endif
        _unlock();
        errno = EINVAL;
        return -1;
    }

    if (op == FNDK_EPOLL_CTL_ADD && epoll->interest->contains(fd)) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_ctl(epfd:%i, op:%s, fd:%i): EEXIST: op was EPOLL_CTL_ADD, and the supplied file descriptor fd is already registered with this epoll instance.", epfd, __op_to_str(op), fd);
#endif
        _unlock();
        errno = EEXIST;
        return -1;
    }

    // EINVAL: An invalid event type was specified along with EPOLLEXCLUSIVE in events.
    // ????

    if (!event && op != FNDK_EPOLL_CTL_DEL) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_ctl(epfd:%i, op:%s, fd:%i): EINVAL: [extra]: `event` can not be null if `op` isn't EPOLL_CTL_DEL", epfd, __op_to_str(op), fd);
#endif
        _unlock();
        errno = EINVAL;
        return -1;
    }

    if ((op == FNDK_EPOLL_CTL_MOD || op == FNDK_EPOLL_CTL_DEL) && !epoll->interest->contains(fd)) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_ctl(epfd:%i, op:%s, fd:%i): ENOENT: op was EPOLL_CTL_MOD or EPOLL_CTL_DEL, and fd is not registered with this epoll instance.", epfd, __op_to_str(op), fd);
#endif

        _unlock();
        errno = ENOENT;
        return -1;
    }

    if (op == FNDK_EPOLL_CTL_MOD && epoll->interest->at(fd).e.events & FNDK_EPOLLEXCLUSIVE) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_ctl(epfd:%i, op:%s, fd:%i): EINVAL: op was EPOLL_CTL_MOD and the EPOLLEXCLUSIVE flag has previously been applied to this epfd, fd pair.", epfd, __op_to_str(op), fd);
#endif

        _unlock();
        errno = EINVAL;
        return -1;
    }

    if (op == FNDK_EPOLL_CTL_MOD && event->events & FNDK_EPOLLEXCLUSIVE) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_ctl(epfd:%i, op:%s, fd:%i): EINVAL: op was EPOLL_CTL_MOD and events included EPOLLEXCLUSIVE.", epfd, __op_to_str(op), fd);
#endif

        _unlock();
        errno = EINVAL;
        return -1;
    }

    if (fd >= EPOLL_FD_MARGIN && fd < EPOLL_FD_MARGIN + EPOLL_FD_MAX) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_ctl(epfd:%i, op:%s, fd:%i): ELOOP: fd refers to an epoll instance and this EPOLL_CTL_ADD operation would result in a circular loop of epoll instances monitoring one another or a nesting depth of epoll instances greater than 5.", epfd, __op_to_str(op), fd);
#endif

        // fd refers to an epoll instance. while not exactly per spec, but let's easen up our life a bit by
        // not supporting this case
        _unlock();
        errno = ELOOP;
        return -1;
    }

    if (op == FNDK_EPOLL_CTL_ADD || op == FNDK_EPOLL_CTL_MOD) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_ctl(epfd:%i, op:%s, fd:%i): adding/modding fd %i. IN stat:%i, OUT stat:%i", epfd, __op_to_str(op), fd, fd, event->events & FNDK_EPOLLIN, event->events & FNDK_EPOLLOUT);
#endif

        epollElement ele;
        ele.e = *event;
        ele.fd = fd;
        epoll->interest->insert_or_assign(fd, ele);
        _unlock();
        return 0;
    }

    epoll->interest->erase(fd);
    _unlock();
    return 0;
}

int fndk_epoll_wait(int epfd, struct fndk_epoll_event *events, int maxevents, int timeout) {
#ifdef DEBUG_EPOLL
    ALOGD("fndk_epoll_wait: epfd: %i; events: 0x%x; maxevents: %i, timeout: %i", epfd, events, maxevents, timeout);
#endif

    // fd out of our defined bounds
    if (epfd < EPOLL_FD_MARGIN || epfd >= EPOLL_FD_MARGIN + EPOLL_FD_MAX) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_wait: epoll fd out of bounds");
#endif

        errno = EBADF;
        return -1;
    }

    if (maxevents <= 0) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_wait: maxevents <= 0");
#endif

        errno = EINVAL;
        return -1;
    }

    _lock();

    _epoll_fd_internal * fd = &epoll_fd_pool[epfd - EPOLL_FD_MARGIN];

    if (fd->fd != epfd) {
#ifdef DEBUG_EPOLL
        ALOGD("fndk_epoll_wait: epoll fd not found in pool");
#endif

        _unlock();
        errno = EINVAL;
        return -1;
    }

    uint64_t time_started = AFN_timeMillis();
    int eventsReported = 0;

    for (;;) {
        for (auto & e : *fd->interest) {
            bool is_readable, is_writeable;

            if (is_eventfd(e.first)) {
                fndk_eventfd_status(e.first, &is_readable, &is_writeable);
            } else if (is_pipe(e.first)) {
                fndk_pipe_status(e.first, &is_readable, &is_writeable, true);
            } else {
#ifdef DEBUG_EPOLL
                ALOGD("fndk_epoll_wait: unknown fd type for fd %i", e.first);
#endif
                continue;
            }

            if ((e.second.e.events & FNDK_EPOLLIN && is_readable) || (e.second.e.events & FNDK_EPOLLOUT && is_writeable)) {
                if (eventsReported >= maxevents) {
                    break;
                }

                memcpy(&events[eventsReported], &e.second.e, sizeof(fndk_epoll_event));
                events[eventsReported].events = 0;
                if (e.second.e.events & FNDK_EPOLLIN && is_readable) events[eventsReported].events |= FNDK_EPOLLIN;
                if (e.second.e.events & FNDK_EPOLLOUT && is_writeable) events[eventsReported].events |= FNDK_EPOLLOUT;

#ifdef DEBUG_EPOLL
                int __x = (e.second.e.events & FNDK_EPOLLIN && is_readable);
                int __y = (e.second.e.events & FNDK_EPOLLOUT && is_writeable);
                if (__x && __y) {
                    ALOGD("fndk_epoll_wait: reporting events IN+OUT for fd %i", e.first);
                } else if (__x) {
                    ALOGD("fndk_epoll_wait: reporting event IN for fd %i", e.first);
                } else if (__y) {
                    ALOGD("fndk_epoll_wait: reporting event OUT for fd %i", e.first);
                }
#endif
                eventsReported++;
            }
        }

        if (timeout == 0) goto done;
        if (eventsReported >= maxevents) goto done;
        if (timeout == -1 && eventsReported > 0) goto done;

        if (timeout != -1) {
            if (AFN_timeMillis() - time_started > timeout) goto done;
        }

        _unlock();
        usleep(4167); // give a chance for other threads to add new FDs to pool / 1/4 of a frame at 60fps
        _lock();
    }

done:
    _unlock();
    return eventsReported;
}

