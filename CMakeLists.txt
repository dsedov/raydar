cmake_minimum_required(VERSION 3.10)
project(raydar)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)



# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)

# Explicitly set libpng paths
set(PNG_INCLUDE_DIR "/opt/homebrew/opt/libpng/include")
set(PNG_LIBRARY "/opt/homebrew/opt/libpng/lib/libpng.dylib")

# Verify paths exist
if(NOT EXISTS ${PNG_INCLUDE_DIR} OR NOT EXISTS ${PNG_LIBRARY})
    message(FATAL_ERROR "libpng paths not found. Please check your installation.")
endif()

# Debug: Print PNG paths
message(STATUS "PNG_INCLUDE_DIR: ${PNG_INCLUDE_DIR}")
message(STATUS "PNG_LIBRARY: ${PNG_LIBRARY}")

# Explicitly set PNG_PNG_INCLUDE_DIR to avoid picking up wrong headers
set(PNG_PNG_INCLUDE_DIR ${PNG_INCLUDE_DIR})

# Find PNG package
find_package(PNG REQUIRED)

# Override PNG version string
set(PNG_VERSION_STRING "1.6.43" CACHE STRING "Forcing correct PNG version" FORCE)

# Debug: Print PNG variables
message(STATUS "PNG_FOUND: ${PNG_FOUND}")
message(STATUS "PNG_INCLUDE_DIRS: ${PNG_INCLUDE_DIRS}")
message(STATUS "PNG_LIBRARIES: ${PNG_LIBRARIES}")
message(STATUS "PNG_VERSION_STRING: ${PNG_VERSION_STRING}")

set(USD_ROOT /Users/dennissedov/Documents/Dev/pxr)
include_directories(${USD_ROOT}/include)
link_directories(${USD_ROOT}/lib)
find_library(USD_LIB usd_usd PATHS ${USD_ROOT}/lib)
find_library(USD_USDGeom_LIB usd_usdGeom PATHS ${USD_ROOT}/lib)

# Add the executable
add_executable(raydar src/main.cpp)

# Include directories
target_include_directories(raydar PRIVATE include ${PNG_INCLUDE_DIR})
target_include_directories(raydar PRIVATE src)


# Link libraries
target_link_libraries(raydar PRIVATE ${PNG_LIBRARY} ${USD_LIB} ${USD_USDGeom_LIB})


# Add compile definitions if needed
target_compile_definitions(raydar PRIVATE ${PNG_DEFINITIONS})

# Print link libraries for debugging
get_target_property(LINKED_LIBS raydar LINK_LIBRARIES)
message(STATUS "Linked libraries: ${LINKED_LIBS}")