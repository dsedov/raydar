cmake_minimum_required(VERSION 3.10)
project(raydar)

# Specify the C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)

# Find libpng package
find_package(PNG REQUIRED)

# Add the executable
add_executable(raydar src/main.c)

# Include directories
target_include_directories(raydar PRIVATE include ${PNG_INCLUDE_DIRS})

# Link libraries
target_link_libraries(raydar PRIVATE ${PNG_LIBRARIES})

# Add compile definitions if needed
target_compile_definitions(raydar PRIVATE ${PNG_DEFINITIONS})