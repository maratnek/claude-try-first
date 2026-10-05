cmake_minimum_required(VERSION 3.24)

# EMSCRIPTEN is only defined once the toolchain file is processed inside project(), too late for this check.
if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "Emscripten" AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan_provider.cmake")
endif()

project(FlightGame CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    include(FetchContent)
    set(PLATFORM Web CACHE STRING "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    include(FetchContent)
    # raylib 5.5 has no native iOS backend; its SDL2 backend is the closest path.
    set(PLATFORM SDL CACHE STRING "" FORCE)
    set(OPENGL_VERSION "ES 2.0" CACHE STRING "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.9
        GIT_SHALLOW TRUE
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(SDL2)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
    # raylib's CMake compiles raudio.c as plain C, but miniaudio includes AVFoundation (Objective-C) on iOS.
    set_source_files_properties("${raylib_SOURCE_DIR}/src/raudio.c"
        TARGET_DIRECTORY raylib PROPERTIES COMPILE_FLAGS "-x objective-c")
else()
    find_package(raylib REQUIRED)
endif()

add_executable(FlightGame
    src/main.cpp
    src/audio.cpp
    src/flight.cpp
    src/input.cpp
    src/level.cpp
    src/menu.cpp
    src/safe_area.cpp
    src/settings.cpp
    src/objects/glb_nodes.cpp
    src/objects/break_data.cpp
    src/objects/plane.cpp
    src/objects/world.cpp
    src/objects/scatter.cpp
    src/objects/clouds.cpp
    src/objects/smoke.cpp
    src/objects/streaks.cpp
    src/objects/rain.cpp
    src/objects/snow.cpp
    src/objects/debris.cpp
    src/objects/character.cpp
)
target_include_directories(FlightGame PRIVATE src)
target_link_libraries(FlightGame PRIVATE raylib)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set_target_properties(FlightGame PROPERTIES OUTPUT_NAME index)
    target_link_options(FlightGame PRIVATE
        -sUSE_GLFW=3
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=134217728
        -sSTACK_SIZE=1048576
        -sEXPORTED_RUNTIME_METHODS=HEAPF32
        --preload-file "${CMAKE_SOURCE_DIR}/assets@/assets"
        --shell-file "${CMAKE_SOURCE_DIR}/web/shell.html"
    )
else()
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(IOS_ASSET_CATALOG "${CMAKE_SOURCE_DIR}/ios/Assets.xcassets")
        target_sources(FlightGame PRIVATE "${IOS_ASSET_CATALOG}")
        set_target_properties(FlightGame PROPERTIES
            RESOURCE "${IOS_ASSET_CATALOG}"
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/ios/Info.plist.in"
            MACOSX_BUNDLE_GUI_IDENTIFIER com.flightgame.FlightGame
            MACOSX_BUNDLE_BUNDLE_NAME FlightGame
            MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0
            MACOSX_BUNDLE_BUNDLE_VERSION 1
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.flightgame.FlightGame
        )
        target_link_libraries(FlightGame PRIVATE
            SDL2::SDL2main SDL2::SDL2
            "-framework UIKit" "-framework OpenGLES" "-framework AudioToolbox"
            "-framework CoreAudio" "-framework AVFoundation" "-framework CoreGraphics"
            "-framework QuartzCore" "-framework Foundation"
        )
    endif()
    # Copy assets next to the built executable so it finds them via
    # GetApplicationDirectory() regardless of the working directory the
    # launcher (terminal, VSCode CMake Tools, Xcode, ...) starts it from.
    add_custom_command(TARGET FlightGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:FlightGame>/assets"
    )
endif()
