cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0074 NEW) 

project(WizardRoguelike)

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -g")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)


find_package(SDL2_ttf REQUIRED)


find_package(SDL2_image REQUIRED)


add_executable(WizardRoguelike
    src/main.cpp
    src/menu.cpp
    src/character_select.cpp
    src/character.cpp
    src/level.cpp
    src/enemy.cpp
    src/utils.cpp
    src/ui.cpp
    src/visibility.cpp
    src/projectile.cpp
    src/asset_manager.cpp
    src/orbital_missile.cpp
)

target_link_libraries(WizardRoguelike PUBLIC
    SDL2::SDL2main          # Handles main() differences FIRST
    SDL2::SDL2              # Main SDL2 library target SECOND
    SDL2_image::SDL2_image  # SDL2_image library target
    SDL2_ttf::SDL2_ttf      # SDL2_ttf library target
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Define the path to the Windows libs bin directory (relative to source)
    set(WINDOWS_BIN_DIR "${CMAKE_SOURCE_DIR}/windows_libs/x86_64-w64-mingw32/bin")

    # List of required DLLs (You may need to check dependencies, e.g., libpng, zlib)
# Corrected list of required DLLs with full paths
set(REQUIRED_DLLS
"${CMAKE_SOURCE_DIR}/windows_libs/SDL2-2.32.4/x86_64-w64-mingw32/bin/SDL2.dll"
"${CMAKE_SOURCE_DIR}/windows_libs/SDL2_image-2.8.8/x86_64-w64-mingw32/bin/SDL2_image.dll"
"${CMAKE_SOURCE_DIR}/windows_libs/SDL2_ttf-2.24.0/x86_64-w64-mingw32/bin/SDL2_ttf.dll"

        # MinGW Runtime DLLs (Using verified paths from 'find' command)
        "/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libgcc_s_seh-1.dll"  # <-- Verified path
        "/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libstdc++-6.dll"    # <-- Verified path
        "/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll"   

)

    # Copy DLLs to the build directory where the executable is created
    add_custom_command(TARGET WizardRoguelike POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${REQUIRED_DLLS}
        $<TARGET_FILE_DIR:WizardRoguelike>
        COMMENT "Copying required DLLs to build directory"
    )

    # Optional: Install rule to copy executable and DLLs
    install(TARGETS WizardRoguelike DESTINATION bin)
    install(FILES ${REQUIRED_DLLS} DESTINATION bin)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/" DESTINATION .) # Install assets too
endif()