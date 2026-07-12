#include "linux/fndk_unistd.h"

#include "sys/unistd.h"

#include "linux/fndk_epoll.h"
#include "linux/fndk_eventfd.h"
#include "linux/fndk_pipe.h"

ssize_t fndk_read(int fd, void *buf, size_t count) {
    if (is_eventfd(fd)) {
        return fndk_eventfd_read(fd, buf, count);
    } else if (is_pipe(fd)) {
        return fndk_pipe_read(fd, buf, count);
    } else {
        // not eventfd or pipe, fallback to a normal read
        return read(fd, buf, count);
    }
}

ssize_t fndk_write(int fd, const void *buf, size_t count) {
    if (is_eventfd(fd)) {
        return fndk_eventfd_write(fd, buf, count);
    } else if (is_pipe(fd)) {
        return fndk_pipe_write(fd, buf, count);
    } else {
        // not eventfd or pipe, fallback to a normal write
        return write(fd, buf, count);
    }
}

int fndk_close(int fd) {
    if (is_epoll(fd)) {
        return fndk_epoll_close(fd);
    } else if (is_eventfd(fd)) {
        return fndk_eventfd_close(fd);
    } else if (is_pipe(fd)) {
        return fndk_pipe_close(fd);
    } else {
        return close(fd);
    }
}
