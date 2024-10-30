cmake_minimum_required(VERSION 3.16)

project(raydar)
# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

if(WIN32)  
    set(CMAKE_PREFIX_PATH "C:/Qt/6.8.0/msvc2022_64/lib/cmake")
endif()
find_package(Qt6 COMPONENTS Core Widgets OpenGLWidgets REQUIRED)


set(CMAKE_AUTOMOC ON)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()


# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)

# Add compiler flags for auto-vectorization and optimization
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvectorize -fslp-vectorize")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -funroll-loops")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-class-conversion -Wno-#pragma-messages")
elseif(WIN32)   
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /arch:AVX2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /GL")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:16777216")
endif()

# Enable OpenMP if available
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

set(USD_ROOT "c:/Dev/pxr")



include_directories(${USD_ROOT}/include)
link_directories(${USD_ROOT}/lib)

# Find all USD libraries
if(WIN32)
    file(GLOB USD_LIBRARIES "${USD_ROOT}/lib/usd_*.lib")
else()
    file(GLOB USD_LIBRARIES "${USD_ROOT}/lib/libusd_*.dylib")
endif()

# Add the new render_window.cpp to your source files
set(SOURCES
    src/usd/camera.cpp
    src/usd/geo.cpp
    src/usd/light.cpp
    src/usd/material.cpp
    src/main.cpp
    src/render.cpp
    src/ui/render_window.cpp 
    src/ui/components/uispectralgraph.cpp
    src/ui/components/uiopenglimage.cpp
    src/ui/components/uidropdownmenu.cpp
    src/ui/components/uifloat.cpp
    src/ui/components/uiint.cpp 
    src/ui/components/uiint2.cpp
    src/ui/components/uiusdtreeview.cpp
    src/ui/components/usd_tree_component.cpp
    src/ui/components/spd_file_list_component.cpp
)
add_executable(raydar ${SOURCES})

# Include directories   
target_include_directories(raydar PRIVATE include ${PNG_INCLUDE_DIR})
target_include_directories(raydar PRIVATE src)
target_include_directories(raydar PRIVATE ${Qt6Core_INCLUDE_DIRS})
target_include_directories(raydar PRIVATE ${Qt6Gui_INCLUDE_DIRS})
target_include_directories(raydar PRIVATE ${Qt6Widgets_INCLUDE_DIRS})
target_include_directories(raydar PRIVATE ${Qt6OpenGLWidgets_INCLUDE_DIRS})
target_include_directories(raydar PRIVATE ${Qt6OpenGL_INCLUDE_DIRS})

# Link libraries
target_link_libraries(raydar PRIVATE 
                                PNG::PNG 
                                ${USD_LIBRARIES}
                                ${Qt6Core_LIBRARIES}
                                ${Qt6Gui_LIBRARIES}
                                ${Qt6Widgets_LIBRARIES}
                                ${Qt6OpenGLWidgets_LIBRARIES}
                                ${Qt6OpenGL_LIBRARIES})

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
            /fp:fast
            /wd4100  # Disable warning on unreferenced formal parameter
            /wd4458  # Disable warning on class member hiding
        )
endif()

# Function to copy DLLs after build
function(copy_dll_files TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        # Copy PNG DLL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:PNG::PNG>
            $<TARGET_FILE_DIR:${TARGET_NAME}>

    )
endfunction()
copy_dll_files(${PROJECT_NAME})