cmake_minimum_required(VERSION 3.20)

project(
    labwork10
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(lib lib/include)

add_library(labwork_lib STATIC
    lib/src/DataBase.cpp
    lib/src/DBTypes.cpp
    lib/src/ValKeyClient.cpp
    lib/src/ValKeyServer.cpp
    lib/src/utils.cpp
)

add_subdirectory(bin)

# enable_testing()
# add_subdirectory(tests)

# include(tests/coverage.cmake)