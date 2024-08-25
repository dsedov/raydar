cmake_minimum_required(VERSION 3.10)
project(raydar)
#set(CMAKE_BUILD_TYPE Debug)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()
# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)

# Add compiler flags for auto-vectorization and optimization
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvectorize -fslp-vectorize")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -funroll-loops")
    
    # Enable OpenMP if available
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
elseif(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /arch:AVX2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /GL")
    
    # Enable OpenMP if available
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
endif()


if (WIN32)
    set(PNG_INCLUDE_DIR "D:/dev/vcpkg/packages/libpng_x64-windows/include")
    set(PNG_LIBRARY "D:/dev/vcpkg/packages/libpng_x64-windows/lib/libpng16.lib")
    set(ZLIB_INCLUDE_DIR "D:/dev/vcpkg/packages/zlib_x64-windows/include")
    set(ZLIB_LIBRARY "D:/dev/vcpkg/packages/zlib_x64-windows/lib/zlib.dylib")
    set(PNG_PNG_INCLUDE_DIR ${PNG_INCLUDE_DIR})
    set(USD_ROOT "D:/Dev/pxr")

    
else()
    set(PNG_INCLUDE_DIR "/opt/homebrew/opt/libpng/include")
    set(PNG_LIBRARY "/opt/homebrew/opt/libpng/lib/libpng.dylib")
    set(PNG_PNG_INCLUDE_DIR ${PNG_INCLUDE_DIR})
    set(USD_ROOT "/Users/dennissedov/Dev/pxr")
endif()

find_package(PNG REQUIRED)
include_directories(${USD_ROOT}/include)
link_directories(${USD_ROOT}/lib)

find_library(USD_LIB usd_usd PATHS ${USD_ROOT}/lib)
find_library(USD_USDGeom_LIB usd_usdGeom PATHS ${USD_ROOT}/lib)
find_library(USD_TF_LIB usd_tf PATHS ${USD_ROOT}/lib)
find_library(USD_SDF_LIB usd_sdf PATHS ${USD_ROOT}/lib)
find_library(USD_VT_LIB usd_vt PATHS ${USD_ROOT}/lib)
find_library(USD_GF_LIB usd_gf PATHS ${USD_ROOT}/lib)
find_library(USD_USDShade_LIB usd_usdShade PATHS ${USD_ROOT}/lib)
find_library(USD_USD_LUX_LIB usd_usdLux PATHS ${USD_ROOT}/lib)

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
                                ${USD_GF_LIB}
                                ${USD_USDShade_LIB}
                                ${USD_USD_LUX_LIB})

# Add compile definitions if needed
target_compile_definitions(raydar PRIVATE ${PNG_DEFINITIONS} NOMINMAX WIN32_LEAN_AND_MEAN)
if (WIN32)
    target_compile_options(raydar PRIVATE
            /W3
            /wd4244
            /wd4267
            /wd4305
            /wd4996
            /wd4275
            /wd4251
        )
endif()
# Print link libraries for debugging
get_target_property(LINKED_LIBS raydar LINK_LIBRARIES)
message(STATUS "Linked libraries: ${LINKED_LIBS}")