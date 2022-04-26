#pragma once

#ifndef NDEBUG
inline constexpr bool isDebugBuild = true;
#else
inline constexpr bool isDebugBuild = false;
#endif

extern bool debugEnabled;

void setupDebug() noexcept;
