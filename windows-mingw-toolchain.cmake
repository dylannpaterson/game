# windows-mingw-toolchain.cmake (Revised)
# Toolchain file for cross-compiling from Linux (WSL) to Windows using MinGW-w64

# Target system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64) # Or i686 if you target 32-bit Windows

# Specify the cross-compilers
# Adjust paths if necessary, though 'apt install' usually puts them in the PATH
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres) # For Windows resources, if any

# --- Refined Search Path Strategy ---

# Where to look for target environment tools (ONLY needed if not in PATH)
# set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER) # Don't look in host root path for programs

# Where to look for libraries and headers for the target system
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# --- More Specific Path Specification ---
# Get the directory containing this toolchain file
get_filename_component(TOOLCHAIN_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)

# Define the base path to the windows_libs directory
set(WINDOWS_LIBS_BASE "${TOOLCHAIN_DIR}/windows_libs")

# Define paths to the ROOT directory of each specific library installation
# These directories should contain the 'include' and 'lib' subdirectories for the target
set(SDL2_ROOT "${WINDOWS_LIBS_BASE}/SDL2-2.32.4/x86_64-w64-mingw32")
set(SDL2_IMAGE_ROOT "${WINDOWS_LIBS_BASE}/SDL2_image-2.8.8/x86_64-w64-mingw32")
set(SDL2_TTF_ROOT "${WINDOWS_LIBS_BASE}/SDL2_ttf-2.24.0/x86_64-w64-mingw32")

# Add these specific paths to CMAKE_PREFIX_PATH.
# CMake's find_package command will search these prefixes for SDL2Config.cmake, etc.
list(APPEND CMAKE_PREFIX_PATH ${SDL2_ROOT} ${SDL2_IMAGE_ROOT} ${SDL2_TTF_ROOT})

# Also add them to CMAKE_FIND_ROOT_PATH for broader searching if needed
list(APPEND CMAKE_FIND_ROOT_PATH ${SDL2_ROOT} ${SDL2_IMAGE_ROOT} ${SDL2_TTF_ROOT})

# --- End Refined Search Path Strategy ---

# Set runtime library path hint for finding DLLs if needed during build (less common)
# set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/bin")