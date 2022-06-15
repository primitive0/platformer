#pragma once

#include "os_type.h"

#if defined(__COMPILES_LINUX__)
#include <sched.h>

inline void threadYield() noexcept {
    sched_yield();
}
#elif defined(__COMPILES_WINDOWS__)
#include "windows/win_api.h"

inline void threadYield() noexcept {
    SwitchToThread();
}
#else
inline void threadYield() noexcept {}
#endif
