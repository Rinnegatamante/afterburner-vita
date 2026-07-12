/*
 * FalsoNDK_Utils.cpp
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 * Copyright (C) 2026 Ellie J Turner
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file for details.
 */

#include "FalsoNDK_Utils.h"

#include <sys/time.h>
#include <psp2/kernel/clib.h>

uint64_t AFN_timeMillis() {
    struct timeval te{};
    gettimeofday(&te, nullptr);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

__attribute__((weak))
void fndk_log(int severity, const char * message) {
    static const char * const prefixes[] = { "D", "W", "E", "F" };
    sceClibPrintf("%s/FalsoNDK: %s\n", prefixes[severity], message);
}

#define LOG_PRINT(severity, fmt) \
    char log_buffer[2048]; \
    va_list list; \
    va_start(list, fmt); \
    sceClibVsnprintf(log_buffer, sizeof(log_buffer), fmt, list); \
    va_end(list); \
    fndk_log(severity, log_buffer);

void LOG_ALWAYS_FATAL_IF(bool cond, const char * fmt, ...) {
    if (!cond) return;
    LOG_PRINT(FALSONDK_LOG_FATAL, fmt);
    sceClibAbort();
}

__attribute__((noreturn))
void LOG_ALWAYS_FATAL(const char * fmt, ...) {
    LOG_PRINT(FALSONDK_LOG_FATAL, fmt);
    sceClibAbort();
}

void ALOGE(const char * fmt, ...) {
    LOG_PRINT(FALSONDK_LOG_ERROR, fmt);
}

void ALOGW(const char * fmt, ...) {
    LOG_PRINT(FALSONDK_LOG_WARN, fmt);
}

void ALOGD(const char * fmt, ...) {
    LOG_PRINT(FALSONDK_LOG_DEBUG, fmt);
}
