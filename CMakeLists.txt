cmake_minimum_required(VERSION 3.24)

set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan_provider.cmake")

project(FlightGame CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

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
