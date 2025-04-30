cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0074 NEW) 

project(WizardRoguelike)

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -g")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS})

find_package(SDL2_ttf REQUIRED)
include_directories(${SDL2_TTF_INCLUDE_DIRS})
link_directories(${SDL2_TTF_LIBRARY_DIRS})

find_package(SDL2_image REQUIRED)
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
link_directories(${SDL2_IMAGE_LIBRARY_DIRS})

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

target_link_libraries(WizardRoguelike
    ${SDL2_LIBRARIES}
    SDL2::SDL2main
    ${SDL2_TTF_LIBRARIES}
    SDL2_ttf # While redundant with the above, sometimes necessary for clarity
    ${SDL2_IMAGE_LIBRARIES}
    SDL2_image # Similar to SDL2_ttf
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Define the path to the Windows libs bin directory (relative to source)
    set(WINDOWS_BIN_DIR "${CMAKE_SOURCE_DIR}/windows_libs/x86_64-w64-mingw32/bin")

    # List of required DLLs (You may need to check dependencies, e.g., libpng, zlib)
    set(REQUIRED_DLLS
        "${WINDOWS_BIN_DIR}/SDL2.dll"
        "${WINDOWS_BIN_DIR}/SDL2_image.dll"
        "${WINDOWS_BIN_DIR}/SDL2_ttf.dll"
        # Add other DLLs found in windows_libs/../bin like:
        "${WINDOWS_BIN_DIR}/libpng16-16.dll"
        "${WINDOWS_BIN_DIR}/zlib1.dll"
        "${WINDOWS_BIN_DIR}/libjpeg-9.dll"  # Check exact names!
        "${WINDOWS_BIN_DIR}/libtiff-5.dll"
        "${WINDOWS_BIN_DIR}/libwebp-7.dll"
        "${WINDOWS_BIN_DIR}/libfreetype-6.dll"
        # ... any others?
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