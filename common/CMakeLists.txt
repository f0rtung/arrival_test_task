project (Common)
cmake_minimum_required (VERSION 3.1)
set(CMAKE_CXX_STANDARD 14)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Use default cmake build type: Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -std=c++11")

set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_PATH})

file(GLOB SRC_LIST
    ./src/*.*
)

set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_PATH})

set(MODULE_NAME ${PROJECT_NAME})

add_library(${MODULE_NAME} STATIC ${SRC_LIST})

target_link_libraries(
    ${PROJECT_NAME}
    log4cplus
)
