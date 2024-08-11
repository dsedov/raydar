cmake_minimum_required(VERSION 3.10)
project(raydar)

set(CMAKE_OSX_ARCHITECTURES "arm64")
# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)



set(PNG_INCLUDE_DIR "/opt/homebrew/opt/libpng/include")
set(PNG_LIBRARY "/opt/homebrew/opt/libpng/lib/libpng.dylib")

set(PNG_PNG_INCLUDE_DIR ${PNG_INCLUDE_DIR})
find_package(PNG REQUIRED)


# OPENUSD
set(USD_ROOT /Users/dennissedov/Dev/pxr)


include_directories(${USD_ROOT}/include)
link_directories(${USD_ROOT}/lib)
find_library(USD_LIB usd_usd PATHS ${USD_ROOT}/lib)
find_library(USD_USDGeom_LIB usd_usdGeom PATHS ${USD_ROOT}/lib)
find_library(USD_TF_LIB usd_tf PATHS ${USD_ROOT}/lib)
find_library(USD_SDF_LIB usd_sdf PATHS ${USD_ROOT}/lib)
find_library(USD_VT_LIB usd_vt PATHS ${USD_ROOT}/lib)
find_library(USD_GF_LIB usd_gf PATHS ${USD_ROOT}/lib)

# Add the executable
add_executable(raydar src/main.cpp)

# Include directories
target_include_directories(raydar PRIVATE include ${PNG_INCLUDE_DIR})
target_include_directories(raydar PRIVATE src)


# Link libraries
target_link_libraries(raydar PRIVATE 
                                ${PNG_LIBRARY} 
                                ${USD_LIB} 
                                ${USD_USDGeom_LIB} 
                                ${USD_USDStage_LIB} 
                                ${USD_TF_LIB} 
                                ${USD_SDF_LIB}
                                ${USD_VT_LIB}
                                ${USD_GF_LIB})


# Add compile definitions if needed
target_compile_definitions(raydar PRIVATE ${PNG_DEFINITIONS} NOMINMAX WIN32_LEAN_AND_MEAN)

# Print link libraries for debugging
get_target_property(LINKED_LIBS raydar LINK_LIBRARIES)
message(STATUS "Linked libraries: ${LINKED_LIBS}")