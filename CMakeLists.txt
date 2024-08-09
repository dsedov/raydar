cmake_minimum_required(VERSION 3.10)
project(raydar)

if(WIN32)
  set(CMAKE_GENERATOR_PLATFORM x64)
endif()
# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)

# ZLIB
if (WIN32)
    set(ZLIB_INCLUDE_DIR "D:/dev/vcpkg/packages/zlib_x64-windows/include")
    set(ZLIB_LIBRARY "D:/dev/vcpkg/packages/zlib_x64-windows/lib/zlib.lib")
endif()

# LIB PNG
if (WIN32)  
    set(PNG_INCLUDE_DIR "D:/dev/vcpkg/packages/libpng_x64-windows/include")
    set(PNG_LIBRARY "D:/dev/vcpkg/packages/libpng_x64-windows/lib/libpng16.lib") 
    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
else()
    set(PNG_INCLUDE_DIR "/opt/homebrew/opt/libpng/include")
    set(PNG_LIBRARY "/opt/homebrew/opt/libpng/lib/libpng.dylib")
endif()
set(PNG_PNG_INCLUDE_DIR ${PNG_INCLUDE_DIR})
find_package(PNG REQUIRED)


# OPENUSD
if (WIN32)  
    set(USD_ROOT D:/dev/pxr)
else()
    set(USD_ROOT /Users/dennissedov/Documents/Dev/pxr)
endif()

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
target_compile_definitions(raydar PRIVATE ${PNG_DEFINITIONS} NOMINMAX WIN32_LEAN_AND_MEAN)

# Print link libraries for debugging
get_target_property(LINKED_LIBS raydar LINK_LIBRARIES)
message(STATUS "Linked libraries: ${LINKED_LIBS}")