#pragma once

#include <stdint.h>
#include <sys/fcntl.h>

#define FNDK_EPOLL_CLOEXEC O_CLOEXEC
#define FNDK_EPOLL_CTL_ADD 1
#define FNDK_EPOLL_CTL_DEL 2
#define FNDK_EPOLL_CTL_MOD 3
#define FNDK_EPOLLIN 0x00000001
#define FNDK_EPOLLPRI 0x00000002
#define FNDK_EPOLLOUT 0x00000004
#define FNDK_EPOLLERR 0x00000008
#define FNDK_EPOLLHUP 0x00000010
#define FNDK_EPOLLNVAL 0x00000020
#define FNDK_EPOLLRDNORM 0x00000040
#define FNDK_EPOLLRDBAND 0x00000080
#define FNDK_EPOLLWRNORM 0x00000100
#define FNDK_EPOLLWRBAND 0x00000200
#define FNDK_EPOLLMSG 0x00000400
#define FNDK_EPOLLRDHUP 0x00002000
#define FNDK_EPOLLEXCLUSIVE (1U << 28)
#define FNDK_EPOLLWAKEUP (1U << 29)
#define FNDK_EPOLLONESHOT (1U << 30)
#define FNDK_EPOLLET (1U << 31)

#ifdef __cplusplus
extern "C" {
#endif

typedef union fndk_epoll_data {
    void    *ptr;
    int      fd;
    uint32_t u32;
    uint64_t u64;
} fndk_epoll_data_t;

struct fndk_epoll_event {
    uint32_t     events;    /* Epoll events */
    fndk_epoll_data_t data;      /* User data variable */
};

int fndk_epoll_wait(int epfd, struct fndk_epoll_event *events, int maxevents, int timeout);

int fndk_epoll_create(int size);

int fndk_epoll_create1(int flags);

int fndk_epoll_close(int epfd);

bool is_epoll(int fd);

int fndk_epoll_ctl(int epfd, int op, int fd, struct fndk_epoll_event *event);

#ifdef __cplusplus
};
#endif
