/*
 * FalsoNDK_Utils.h
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 * Copyright (C) 2026 Ellie J Turner
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    __typeof__(exp) _rc;                   \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })

//#define DEBUG_EPOLL 1
//#define DEBUG_PIPEFD 1
//#define DEBUG_CALLBACKS 1
//#define DEBUG_POLL_AND_WAKE 1

#define FALSONDK_LOG_DEBUG 0
#define FALSONDK_LOG_WARN  1
#define FALSONDK_LOG_ERROR 2
#define FALSONDK_LOG_FATAL 3

#ifdef __cplusplus
extern "C" {
#endif

uint64_t AFN_timeMillis();

/* Override this symbol to redirect all FalsoNDK log output. */
void fndk_log(int severity, const char * message);

void LOG_ALWAYS_FATAL_IF(bool cond, const char * fmt, ...);
[[noreturn]] void LOG_ALWAYS_FATAL(const char * fmt, ...);
void ALOGE(const char * fmt, ...);
void ALOGW(const char * fmt, ...);
void ALOGD(const char * fmt, ...);

#ifdef __cplusplus
};
#endif
