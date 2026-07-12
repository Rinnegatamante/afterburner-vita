#pragma once

#include <fcntl.h>

/** The eventfd() flag to provide semaphore-like semantics for reads. */
#define FNDK_EFD_SEMAPHORE (1 << 0)
/** The eventfd() flag for a close-on-exec file descriptor. */
#define FNDK_EFD_CLOEXEC O_CLOEXEC
/** The eventfd() flag for a non-blocking file descriptor. */
#define FNDK_EFD_NONBLOCK O_NONBLOCK

#ifdef __cplusplus
extern "C" {
#endif

int fndk_eventfd(unsigned int initval, int flags);
int fndk_eventfd_close(int fd);

bool is_eventfd(int fd);
ssize_t fndk_eventfd_read(int fd, void *buf, size_t count);
ssize_t fndk_eventfd_write(int fd, const void *buf, size_t count);
void fndk_eventfd_status(int fd, bool *is_readable, bool *is_writeable);

#ifdef __cplusplus
};
#endif