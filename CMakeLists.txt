cmake_minimum_required(VERSION 3.24)

# EMSCRIPTEN is only defined once the toolchain file is processed inside project(), too late for this check.
if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "Emscripten")
    set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan_provider.cmake")
endif()

project(FlightGame CXX)

set(CMAKE_CXX_STANDARD 17)
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
else()
    find_package(raylib REQUIRED)
endif()

add_executable(FlightGame
    src/main.cpp
    src/audio.cpp
    src/flight.cpp
    src/level.cpp
    src/objects/plane.cpp
    src/objects/world.cpp
    src/objects/character.cpp
)
target_include_directories(FlightGame PRIVATE src)
target_link_libraries(FlightGame PRIVATE raylib)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    target_link_options(FlightGame PRIVATE
        -sUSE_GLFW=3
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=134217728
        -sSTACK_SIZE=1048576
        --preload-file "${CMAKE_SOURCE_DIR}/assets@/assets"
        --shell-file "${raylib_SOURCE_DIR}/src/minshell.html"
    )
else()
    # Copy assets next to the built executable so it finds them via
    # GetApplicationDirectory() regardless of the working directory the
    # launcher (terminal, VSCode CMake Tools, Xcode, ...) starts it from.
    add_custom_command(TARGET FlightGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:FlightGame>/assets"
    )
endif()
