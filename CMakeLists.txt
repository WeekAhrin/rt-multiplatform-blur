cmake_minimum_required(VERSION 3.20)
project(rt-multiplatform-blur CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BLUR_SOURCES
    blur.cpp
    blur/cpu/cpu.cpp
    blur/gpu/gpu.cpp
)

if(ANDROID)
    add_library(blur STATIC ${BLUR_SOURCES})
    target_link_libraries(blur PUBLIC GLESv3 EGL log)
    target_compile_definitions(blur PRIVATE BLUR_PLATFORM_ANDROID)
else()
    add_library(blur STATIC ${BLUR_SOURCES})
    find_package(OpenGL REQUIRED)
    target_link_libraries(blur PUBLIC OpenGL::GL)
    target_compile_definitions(blur PRIVATE BLUR_PLATFORM_DESKTOP)
endif()

target_include_directories(blur PUBLIC .)
