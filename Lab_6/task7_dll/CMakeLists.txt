cmake_minimum_required(VERSION 3.10)
project(dll_demo C)

set(CMAKE_C_STANDARD 99)

# Create the DLL
add_library(math_library SHARED math_library.c)

# Create the Client executable
add_executable(client client.c)

# Ensure the DLL is in the same directory as the executable for running
add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:math_library>
    $<TARGET_FILE_DIR:client>
)
