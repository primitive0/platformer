#if defined(__linux__) || defined(__LINUX__)
#define __COMPILES_LINUX__
#elif defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN32__) || defined(_MSC_VER)
#define __COMPILES_WINDOWS__
#elif defined(__APPLE__)
#define __COMPILES_MACOS__
#else
#define __COMPILES_UNKNOWN__
#endif
