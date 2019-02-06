project (Client)
cmake_minimum_required (VERSION 3.1)
set(CMAKE_CXX_STANDARD 14)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Use default cmake build type: Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -std=c++11 -pthread")

FIND_PACKAGE(Boost REQUIRED COMPONENTS program_options)

set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_PATH})

link_directories(
    ./../proto/lib/
    ./../common/lib/
)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ./../
)

file(GLOB SRC_LIST
    ./src/*.cpp
    ./src/tcp-client/*.h
    ./src/tcp-client/*.cpp
)

set(MODULE_NAME ${PROJECT_NAME})

add_executable(${MODULE_NAME} ${SRC_LIST})

target_link_libraries(
    ${PROJECT_NAME}
    ${Boost_LIBRARIES}
    event
    log4cplus
    Proto
    Common
)
