cmake_minimum_required(VERSION 3.16)

project(FastPipe C)

# Option to control build type
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Collect source files
file(GLOB LIBFASTPIPE_SOURCES
    libfastpipe/*.c
)

# Create the library
add_library(fastpipe ${LIBFASTPIPE_SOURCES})

# Include directory (for headers)
target_include_directories(fastpipe
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/libfastpipe
)

add_executable(producer examples/producer.c)
add_executable(consumer examples/consumer.c)

target_link_libraries(producer PRIVATE fastpipe)
target_link_libraries(consumer PRIVATE fastpipe)