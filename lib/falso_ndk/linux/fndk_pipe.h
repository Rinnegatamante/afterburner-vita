#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int fndk_pipe(int pipefd[2]);
int fndk_pipe_close(int fd);

bool is_pipe(int fd);
ssize_t fndk_pipe_read(int fd, void *buf, size_t count);
ssize_t fndk_pipe_write(int fd, const void *buf, size_t count);
void fndk_pipe_status(int fd, bool *is_readable, bool *is_writeable, bool consume);

#ifdef __cplusplus
};
#endif
