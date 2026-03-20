#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

#if defined(_WIN32) || defined(_WIN64)
#define OS_WINDOWS 1
#define OS_MACOS 0
#define OS_LINUX 0
#include <windows.h>
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#define F_OK 0 // Windows compatibility
#elif defined(__APPLE__) && defined(__MACH__)
#define OS_WINDOWS 0
#define OS_MACOS 1
#define OS_LINUX 0
#include <mach-o/dyld.h> // For _NSGetExecutablePath
#include <sys/utsname.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#elif defined(__linux__)
#define OS_WINDOWS 0
#define OS_MACOS 0
#define OS_LINUX 1
#include <libgen.h>
#include <sys/utsname.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#else
#error "Unsupported operating system"
#endif

static uint32_t image_count(void) { return 0; } // TODO: Implement this

#endif // !PLATFORM_COMPAT_H
