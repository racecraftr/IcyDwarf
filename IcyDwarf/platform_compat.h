/*
 * platform_compat.h
 *
 * Cross-platform compatibility utilities for IcyDwarf
 * Allows for functionality on windows, macos, and linux
 * Provides platform-independent abstractions for:
 * - Path handling
 * - Executable path detection
 * - Operating system detection
 * - Directory separators (\ on windows, / on linux/macos)
 *
 * Created on: Mar 15, 2026
 *      Author: Avi Gupta
 */

#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

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

/*
 * get_executable_path()
 *
 * Cross-platform function to get the path of the currently running executable.
 *
 * Parameters:
 *   path - pointer to buffer where the executable path will be stored
 *   max_size - maximum size of the buffer
 *
 * Returns:
 *   0 on success, -1 on failure (buffer too small or path retrieval failed)
 */
static int get_executable_path(char *path, size_t max_size) {
	if (path == NULL || max_size == 0) {
		return -1;
	}

#if OS_WINDOWS
	DWORD result = GetModuleFileNameA(NULL, path, (DWORD)max_size);
	if (result == 0 || result >= (DWORD)max_size) {
		return -1;
	}
	return 0;

#elif OS_MACOS
	// macOS-specific approach using _NSGetExecutablePath
	uint32_t size = (uint32_t)max_size;
	if (_NSGetExecutablePath(path, &size) != 0) {
		return -1;
	}
	return 0;

#elif OS_LINUX
	// Linux: try reading from /proc/self/exe (most reliable)
	ssize_t result = readlink("/proc/self/exe", path, max_size - 1);
	if (result == -1) {
		// Fallback: try /proc/self/fd/0
		result = readlink("/proc/self/fd/0", path, max_size - 1);
		if (result == -1) {
			return -1;
		}
	}
	path[result] = '\0';
	return 0;

#endif
}

/*
 * get_os_info()
 *
 * Cross-platform function to get operating system information.
 *
 * Parameters:
 *   os_name - pointer to buffer where OS name will be stored (e.g., "Darwin",
 * "Linux", "Windows") os_name_size - size of os_name buffer os_release -
 * pointer to buffer where OS release/version will be stored os_release_size -
 * size of os_release buffer machine - pointer to buffer where machine
 * architecture will be stored (e.g., "x86_64", "arm64") machine_size - size of
 * machine buffer
 *
 * Returns:
 *   0 on success, -1 on failure
 */
static int get_os_info(char *os_name, size_t os_name_size, char *os_release,
					   size_t os_release_size, char *machine,
					   size_t machine_size) {
	if (os_name == NULL || os_release == NULL || machine == NULL) {
		return -1;
	}

#if OS_WINDOWS
	strncpy(os_name, "Windows", os_name_size - 1);
	os_name[os_name_size - 1] = '\0';

	OSVERSIONINFO info;
	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&info)) {
		snprintf(os_release, os_release_size, "%ld.%ld.%ld",
				 info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber);
	} else {
		strncpy(os_release, "Unknown", os_release_size - 1);
		os_release[os_release_size - 1] = '\0';
	}

#if defined(_M_X64) || defined(__x86_64__)
	strncpy(machine, "x86_64", machine_size - 1);
#elif defined(_M_ARM64) || defined(__aarch64__)
	strncpy(machine, "arm64", machine_size - 1);
#else
	strncpy(machine, "unknown", machine_size - 1);
#endif
	machine[machine_size - 1] = '\0';

#elif OS_MACOS || OS_LINUX
	struct utsname uts;
	if (uname(&uts) != 0) {
		return -1;
	}

	strncpy(os_name, uts.sysname, os_name_size - 1);
	os_name[os_name_size - 1] = '\0';

	strncpy(os_release, uts.release, os_release_size - 1);
	os_release[os_release_size - 1] = '\0';

	strncpy(machine, uts.machine, machine_size - 1);
	machine[machine_size - 1] = '\0';

#endif
	return 0;
}

/*
 * get_os_version_number()
 *
 * Extract major OS version number from OS release string.
 *
 * Parameters:
 *   os_release - OS release string (e.g., "23.3.0", "5.10.0-20")
 *
 * Returns:
 *   Major version number as integer
 */
static int get_os_version_number(const char *os_release) {
	if (os_release == NULL) {
		return 0;
	}

	int major_version = 0;
	char *ptr;
	major_version = (int)strtol(os_release, &ptr, 10);

	return major_version;
}

/*
 * normalize_path()
 *
 * Convert a path to use the correct separator for the current platform.
 * Modifies the path in place.
 *
 * Parameters:
 *   path - pointer to path string (modified in place)
 */
static void normalize_path(char *path) {
	if (path == NULL) {
		return;
	}

#if OS_WINDOWS
	// Convert forward slashes to backslashes
	for (int i = 0; path[i] != '\0'; i++) {
		if (path[i] == '/') {
			path[i] = '\\';
		}
	}
#else
	// Convert backslashes to forward slashes
	for (int i = 0; path[i] != '\0'; i++) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}
#endif
}

/*
 * safe_strcat()
 *
 * Safe string concatenation with buffer overflow protection.
 *
 * Parameters:
 *   dest - destination buffer
 *   src - source string to append
 *   dest_size - total size of destination buffer (including null terminator)
 *
 * Returns:
 *   0 on success, -1 if concatenation would overflow buffer
 */
static int safe_strcat(char *dest, const char *src, size_t dest_size) {
	if (dest == NULL || src == NULL || dest_size == 0) {
		return -1;
	}

	size_t dest_len = strlen(dest);
	size_t src_len = strlen(src);

	if (dest_len + src_len >= dest_size) {
		return -1; // Would overflow
	}

	strcat(dest, src);
	return 0;
}

/*
 * safe_strcpy()
 *
 * Safe string copy with buffer overflow protection.
 *
 * Parameters:
 *   dest - destination buffer
 *   src - source string to copy
 *   dest_size - total size of destination buffer (including null terminator)
 *
 * Returns:
 *   0 on success, -1 if copy would overflow buffer
 */
static int safe_strcpy(char *dest, const char *src, size_t dest_size) {
	if (dest == NULL || src == NULL || dest_size == 0) {
		return -1;
	}

	if (strlen(src) >= dest_size) {
		return -1; // Would overflow
	}

	strcpy(dest, src);
	return 0;
}

/*
 * get_r_home_path()
 *
 * Get the path to R installation directory for the current platform.
 * This function attempts to find R in common installation locations.
 *
 * Parameters:
 *   r_home - pointer to buffer where R home path will be stored
 *   r_home_size - size of r_home buffer
 *
 * Returns:
 *   0 on success, -1 if R home path could not be determined
 */
static int get_r_home_path(char *r_home, size_t r_home_size) {
	if (r_home == NULL || r_home_size == 0) {
		return -1;
	}

	// First, try the R_HOME environment variable
	const char *env_r_home = getenv("R_HOME");
	if (env_r_home != NULL) {
		strncpy(r_home, env_r_home, r_home_size - 1);
		r_home[r_home_size - 1] = '\0';
		return 0;
	}

#if OS_WINDOWS
	// Common Windows R installation paths
	const char *windows_paths[] = {"C:\\Program Files\\R",
								   "C:\\Program Files (x86)\\R", NULL};

	for (int i = 0; windows_paths[i] != NULL; i++) {
		if (access(windows_paths[i], F_OK) == 0) {
			// Try to find the latest R version subdirectory
			// For now, just use the base directory
			strncpy(r_home, windows_paths[i], r_home_size - 1);
			r_home[r_home_size - 1] = '\0';
			return 0;
		}
	}

#elif OS_MACOS
	// macOS R framework paths
	const char *possible_paths[] = {
		"/Library/Frameworks/R.framework/Resources", "/usr/local/lib/R",
		"/Applications/R.app/Contents/MacOS/R", NULL};

	for (int i = 0; possible_paths[i] != NULL; i++) {
		const char *path = possible_paths[i];

		if (access(path, F_OK) == 0) {
			strncpy(r_home, path, r_home_size - 1);
			r_home[r_home_size - 1] = '\0';
			return 0;
		}
	}

#elif OS_LINUX
	// Common Linux R installation paths
	const char *linux_paths[] = {"/usr/lib/R", "/usr/local/lib/R", "/opt/R",
								 NULL};

	for (int i = 0; linux_paths[i] != NULL; i++) {
		if (access(linux_paths[i], F_OK) == 0) {
			strncpy(r_home, linux_paths[i], r_home_size - 1);
			r_home[r_home_size - 1] = '\0';
			return 0;
		}
	}
#endif

	return -1; // Could not determine R home path
}

/*
 * build_full_path()
 *
 * Build a full path from a base directory and a relative path.
 * Handles path separators correctly for the current platform.
 *
 * Parameters:
 *   full_path - pointer to buffer where full path will be stored
 *   full_path_size - size of full_path buffer
 *   base_dir - base directory path
 *   relative_path - relative path to append
 *
 * Returns:
 *   0 on success, -1 on failure
 */
static int build_full_path(char *full_path, size_t full_path_size,
						   const char *base_dir, const char *relative_path) {
	if (full_path == NULL || base_dir == NULL || relative_path == NULL) {
		return -1;
	}

	if (safe_strcpy(full_path, base_dir, full_path_size) != 0) {
		return -1;
	}

	size_t base_len = strlen(full_path);
	if (base_len > 0 && full_path[base_len - 1] != PATH_SEPARATOR) {
		if (safe_strcat(full_path, PATH_SEPARATOR_STR, full_path_size) != 0) {
			return -1;
		}
	}

	if (safe_strcat(full_path, relative_path, full_path_size) != 0) {
		return -1;
	}

	return 0;
}

/*
 * get_icydwarf_base_directory()
 *
 * Extract the base IcyDwarf directory from the executable path.
 * Handles different build configurations (Release, Debug, etc.) across
 * platforms.
 *
 * For instance:
 *   macOS: /path/to/IcyDwarf/Release/IcyDwarf -> /path/to/IcyDwarf/
 *   Linux: /path/to/IcyDwarf/Release/IcyDwarf -> /path/to/IcyDwarf/
 *   Windows: C:\path\to\IcyDwarf\Release\IcyDwarf.exe -> C:\path\to\IcyDwarf\
 *
 * Parameters:
 *   executable_path - full path to the executable
 *   base_dir - pointer to buffer where base directory will be stored
 *   base_dir_size - size of base_dir buffer
 *
 * Returns:
 *   0 on success, -1 on failure
 */
static int get_icydwarf_base_directory(const char *executable_path,
									   char *base_dir, size_t base_dir_size) {
	if (executable_path == NULL || base_dir == NULL || base_dir_size == 0) {
		return -1;
	}

	char temp_path[2048];
	if (safe_strcpy(temp_path, executable_path, sizeof(temp_path)) != 0) {
		return -1;
	}

	int last_separator = -1;
	for (int i = (int)strlen(temp_path) - 1; i >= 0; i--) {
		if (temp_path[i] == '/' || temp_path[i] == '\\') {
			last_separator = i;
			break;
		}
	}

	if (last_separator == -1) {
		return -1;
	}

	temp_path[last_separator] = '\0';
	last_separator = -1;
	for (int i = (int)strlen(temp_path) - 1; i >= 0; i--) {
		if (temp_path[i] == '/' || temp_path[i] == '\\') {
			last_separator = i;
			break;
		}
	}

	if (last_separator == -1) {
		// No parent directory found, use temp_path as-is
		if (safe_strcpy(base_dir, temp_path, base_dir_size) != 0) {
			return -1;
		}
	} else {
		// Keep everything up to and including the last separator
		temp_path[last_separator + 1] = '\0';
		if (safe_strcpy(base_dir, temp_path, base_dir_size) != 0) {
			return -1;
		}
	}

	return 0;
}

#endif // PLATFORM_COMPAT_H
