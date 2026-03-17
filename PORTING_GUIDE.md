# IcyDwarf Cross-Platform Porting Guide

## Summary of Changes for macOS → Windows/Linux Cross-Compatibility

This document outlines all the changes made to make IcyDwarf cross-compatible with Windows and Linux systems. The original code was **macOS-specific** and required modifications in three main areas:

1. **System API calls and path handling**
2. **R framework integration**
3. **Dynamic library loading (dyld) replacement**

---

## 1. New Cross-Platform Compatibility Layer

### File: `platform_compat.h`

A new header file has been created that provides cross-platform abstractions for:

#### 1.1 Executable Path Detection
**Problem**: The original code used `_NSGetExecutablePath()` which is macOS-specific.

**Solution**: 
- **macOS**: Uses `_NSGetExecutablePath()` via `mach-o/dyld.h`
- **Linux**: Uses `readlink("/proc/self/exe")` (Linux `/proc` filesystem)
- **Windows**: Uses `GetModuleFileNameA()` (Windows API)

Function: `get_executable_path(char *path, size_t max_size)`

#### 1.2 Operating System Detection
**Problem**: The original code used `uname()` and `strtod()` to parse OS version, which works on macOS/Linux but not Windows.

**Solution**: 
- **macOS/Linux**: Uses `uname()` struct to get OS name, version, and architecture
- **Windows**: Uses Windows API (`GetVersionEx()`) to get OS version
- Provides unified function to extract version numbers across platforms

Function: `get_os_info()` and `get_os_version_number()`

#### 1.3 R Framework Path Detection
**Problem**: Original code hardcoded R path as `/Library/Frameworks/R.framework/Resources` (macOS-only).

**Solution**: 
- **macOS**: Checks `/Library/Frameworks/R.framework/Resources`
- **Linux**: Checks `/usr/lib/R`, `/usr/local/lib/R`, `/opt/R`
- **Windows**: Checks `C:\Program Files\R`, `C:\Program Files (x86)\R`
- First checks `R_HOME` environment variable before searching

Function: `get_r_home_path(char *r_home, size_t r_home_size)`

#### 1.4 Path Separator Handling
**Problem**: Original code used hardcoded forward slashes `/` in paths, which don't work on Windows.

**Solution**: 
- Defines `PATH_SEPARATOR` macro (Windows: `\`, Unix-like: `/`)
- Provides `build_full_path()` function that constructs paths with correct separators
- Provides `normalize_path()` function to convert paths to platform-native format

#### 1.5 Safe String Operations
**Problem**: Original code used unsafe functions like `strcpy()` and `strcat()` which can cause buffer overflows and are not secure.

**Solution**: 
- `safe_strcpy()`: Performs safe string copying with buffer size checking
- `safe_strcat()`: Performs safe string concatenation with buffer size checking

#### 1.6 Base Directory Extraction
**Problem**: Original code used hardcoded string length calculations to extract the base IcyDwarf directory from executable path:
```c
strncat(idi, path, strlen(path) - 18);  // Removes "Release/IcyDwarf" (18 chars)
// or
strncat(idi, path, strlen(path) - 16);  // Removes "Debug/IcyDwarf" (16 chars)
```

This assumed specific directory structure and filename lengths that vary across platforms.

**Solution**: 
- `get_icydwarf_base_directory()`: Intelligently extracts the base directory by:
  1. Finding the last path separator (either `/` or `\`)
  2. Going up two directory levels to reach the IcyDwarf base directory
  3. Works regardless of platform path separators or exact directory structure

---

## 2. Modified Files

### 2.1 IcyDwarf.c
**Changes**:
- Line 200-202: Replaced hardcoded R path with `get_r_home_path()` call
- Line 210-213: Replaced `_NSGetExecutablePath()` with cross-platform `get_executable_path()`
- Line 216-224: Replaced `uname()` parsing with `get_os_info()` and `get_os_version_number()`

**Before**:
```c
setenv("R_HOME","/Library/Frameworks/R.framework/Resources",1);
if (_NSGetExecutablePath(path, &size) == 0)
    printf("\n");
struct utsname uts;
uname(&uts);
os = strtod(uts.release, &ptr);
```

**After**:
```c
char r_home[1024];
if (get_r_home_path(r_home, sizeof(r_home)) == 0) {
    setenv("R_HOME", r_home, 1);
    printf("R_HOME set to: %s\n", r_home);
}
if (get_executable_path(path, sizeof(path)) == 0) {
    printf("Executable path: %s\n", path);
}
if (get_os_info(os_name, sizeof(os_name), os_release, sizeof(os_release),
                 machine, sizeof(machine)) == 0) {
    os = get_os_version_number(os_release);
}
```

### 2.2 IcyDwarf.h
**Changes**:
- Line 28: Added conditional include of `platform_compat.h` before other includes
- Line 30-33: Made `modifdyld.h` include conditional (`#ifdef __APPLE__`)
- Line 35-48: Added extern declarations for cross-platform functions
- Line 455-470: Replaced hardcoded path truncation with `get_icydwarf_base_directory()` and `build_full_path()`
- Line 1120-1132: Replaced hardcoded path in `read_thermal_output()` function
- Line 1195-1210: Updated `create_output()` function with cross-platform path handling
- Line 1239-1257: Updated `write_output()` function with cross-platform path handling
- Line 1289-1308: Updated `append_output()` function with cross-platform path handling
- Line 1176-1195: Updated `read_input()` function with cross-platform path handling

### 2.3 Thermal.h
**Changes**:
- Line 1088-1105: Replaced hardcoded path construction with `get_icydwarf_base_directory()` and platform-aware path building

---

## 3. Remaining Cross-Platform Issues to Address

The following files contain hardcoded forward slashes in path strings that should ideally be updated to use `PATH_SEPARATOR_STR`:

### 3.1 Files with Hardcoded Paths
1. **Compression.h** (Line 383)
   - `strcat(title,"Outputs/Compression.txt");`
   
2. **Orbit.h** (Lines 508, 685)
   - `strcat(title,"Outputs/Resonances.txt");`
   - `strcat(title,"Outputs/Orbit.txt");`
   
3. **PlanetSystem.h** (Lines 679, 1113, 1307, 1318, 1329)
   - Multiple `strcat(title,"Outputs/...");` calls
   
4. **Crack.h**, **Cryolava.h**, **WaterRock.h**, **WaterRock_ParamExplor.h**
   - Check for similar hardcoded paths

### How to Fix Remaining Issues

For each hardcoded path like:
```c
strcat(title, "Outputs/Thermal.txt");
```

Replace with:
```c
snprintf(temp_path, sizeof(temp_path), "Outputs%s Thermal.txt", PATH_SEPARATOR_STR);
safe_strcat(title, temp_path, sizeof(title));
```

Or use the helper functions:
```c
build_full_path(title, sizeof(title), "Outputs", "Thermal.txt");
```

---

## 4. Removed Dependencies on macOS-Specific Features

### 4.1 Removed modifdyld.h from Non-macOS Builds
The entire file `modifdyld.h` contains:
- `mach-o/loader.h` (Mach-O binary format - macOS only)
- `_dyld_*` functions (macOS dynamic linker APIs)
- `NSAddImage`, `NSLookupSymbolInImage`, etc. (deprecated macOS APIs)

**Solution**: Wrapped include with `#ifdef __APPLE__` so it's only included on macOS.

### 4.2 Platform Detection Macros
Added platform detection at the top of `platform_compat.h`:
```c
#if defined(_WIN32) || defined(_WIN64)
    #define OS_WINDOWS 1
    #define OS_MACOS 0
    #define OS_LINUX 0
#elif defined(__APPLE__) && defined(__MACH__)
    #define OS_WINDOWS 0
    #define OS_MACOS 1
    #define OS_LINUX 0
#elif defined(__linux__)
    #define OS_WINDOWS 0
    #define OS_MACOS 0
    #define OS_LINUX 1
#endif
```

---

## 5. Environment Setup for Different Platforms

### 5.1 Windows Setup

1. **Install R for Windows** from https://cran.r-project.org/bin/windows/base/
   - Default installation path: `C:\Program Files\R\R-4.x.x`
   
2. **Install IPHREEQC** 
   - Download from USGS: https://www.usgs.gov/software/phreeqc-version-3
   
3. **Install SDL2** (for IcyDwarfPlot visualization only)
   - Download from https://github.com/libsdl-org/SDL/releases
   
4. **Set Environment Variables**:
   ```
   R_HOME=C:\Program Files\R\R-4.x.x
   ```

### 5.2 Linux Setup

1. **Install R**:
   ```bash
   sudo apt-get install r-base r-base-dev  # Ubuntu/Debian
   # or
   sudo dnf install R-core R-devel          # Fedora
   # or
   sudo pacman -S r                         # Arch Linux
   ```

2. **Install IPHREEQC**:
   ```bash
   sudo apt-get install libiphreeqc-dev
   # or compile from source
   ```

3. **Install SDL2** (for visualization):
   ```bash
   sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
   ```

### 5.3 macOS Setup (No Changes Needed)

Continue using existing installation procedures. The code is backward-compatible with macOS.

---

## 6. Building on Different Platforms

### 6.1 Compilation Flags

The cross-platform code automatically detects the OS and defines appropriate macros. However, when compiling, ensure:

**Windows**:
```bash
gcc -c IcyDwarf.c -I. -I"C:\Program Files\R\R-4.x.x\include" ...
```

**Linux**:
```bash
gcc -c IcyDwarf.c -I. -I/usr/include/R ...
```

**macOS** (unchanged):
```bash
gcc -c IcyDwarf.c -I. -I/Library/Frameworks/R.framework/Resources/include ...
```

### 6.2 Linking

Ensure you link against:
- libR (R library)
- libiphreeqc (PHREEQC library)
- libm (math library)
- SDL2 libraries (for IcyDwarfPlot only)

---

## 7. Testing and Validation

After porting to a new platform:

1. **Test Executable Path Detection**:
   - Check that the program correctly identifies its own path
   - Verify that input files can be found in `Inputs/` directory
   
2. **Test File I/O**:
   - Verify output files are created in `Outputs/` directory
   - Check that file paths use correct separators on the target OS
   
3. **Test R Integration**:
   - Verify R initialization succeeds
   - Check CHNOSZ commands execute properly
   
4. **Test Thermal Simulation**:
   - Run a sample simulation with default parameters
   - Verify output files are created and contain expected data

---

## 8. Known Limitations and Future Work

### 8.1 Remaining Hard-coded Forward Slashes
Multiple output path constructions still use hardcoded forward slashes that should be replaced with `PATH_SEPARATOR_STR`. These occur in:
- Compression.h
- Orbit.h
- PlanetSystem.h
- Crack.h
- And other module files

### 8.2 Windows Console Issues
The code uses POSIX system calls like `uname()` which don't exist on Windows. These should be further abstracted for proper Windows console compatibility.

### 8.3 Buffer Overflow Risks
Some string operations still use unsafe functions that could be further hardened:
- Remaining `strcpy()` calls
- Remaining `strcat()` calls
- String length assumptions

### 8.4 Dynamic Library Loading
The original `modifdyld.h` functionality for dynamically loading libraries is not replicated for Windows/Linux. If dynamic library loading is needed on these platforms, use `dlopen()/dlsym()` (Linux) or `LoadLibrary()/GetProcAddress()` (Windows).

---

## 9. References

### Documentation
- [C POSIX Library](https://pubs.opengroup.org/onlinepubs/9699919799/)
- [Windows API Documentation](https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-api-list)
- [GCC Predefined Macros](https://gcc.gnu.org/onlinedocs/cpp/Predefined-Macros.html)

### External Libraries
- **R**: https://www.r-project.org/
- **PHREEQC**: https://www.usgs.gov/software/phreeqc-version-3
- **SDL2**: https://www.libsdl.org/

---

## 10. Contact and Support

For issues with cross-platform compilation or porting to new platforms, review:
1. The `platform_compat.h` implementation
2. Compilation warnings for platform-specific issues
3. Output file creation and paths for OS-specific path separator issues

