cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME JPEG)
project(${PROJECT_NAME})

# Set up the compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMPILE_OPTIONS -O3 -Wall -Wextra -Werror -pedantic -pedantic-errors)
set(LINK_OPTIONS "")

# Includes
set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)
include_directories(${COMMON_INCLUDES})

# Utils library
file(GLOB HEADERS_UTILS ${PROJECT_SOURCE_DIR}/include/utils/*.hpp )
file(GLOB SOURCES_UTILS ${PROJECT_SOURCE_DIR}/src/utils/*.cpp )
add_library(Utils ${HEADERS_UTILS} ${SOURCES_UTILS})
target_compile_options(Utils PUBLIC ${COMPILE_OPTIONS})
target_link_options(Utils PUBLIC ${LINK_OPTIONS})

# Encoder
file(GLOB HEADERS_ENCODER ${PROJECT_SOURCE_DIR}/include/encoder/*.hpp)
file(GLOB SOURCES_ENCODER ${PROJECT_SOURCE_DIR}/src/encoder/*.cpp)
add_executable(Encoder ${HEADERS_ENCODER} ${SOURCES_ENCODER})
target_compile_options(Encoder PRIVATE ${COMPILE_OPTIONS})
target_link_options(Encoder PRIVATE ${LINK_OPTIONS})
target_link_libraries(Encoder Utils)

# Decoder
file(GLOB HEADERS_DECODER ${PROJECT_SOURCE_DIR}/include/decoder/*.hpp)
file(GLOB SOURCES_DECODER ${PROJECT_SOURCE_DIR}/src/decoder/*.cpp)
add_executable(Decoder ${HEADERS_DECODER} ${SOURCES_DECODER})
target_compile_options(Decoder PRIVATE ${COMPILE_OPTIONS})
target_link_options(Decoder PRIVATE ${LINK_OPTIONS})
target_link_libraries(Decoder Utils)
