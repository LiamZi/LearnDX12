cmake_minimum_required(VERSION 3.0.0)
project(LearnDX12 VERSION 0.1.0)

set(WINDOW_SYSTEM Win32)

if(WIN32)
    #windows platform
    if(DEFINED TARGET_ARCH)
        if(${TARGET_ARCH} STREQUAL x86)
        elseif(${TARGET_ARCH} STREQUAL X64)
        else()
            set(TARGET_ARCH x64)
            message("Invalid Win32 Architechture, set default architecture to 'x64'...")
        endif()
    else()
        set(TARGET_ARCH x64)
        message("Architechture was not set, set default architecture to 'x64'...")
    endif()

    #Microsoft Visual C++ Compilers
    if(MSVC)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            set(CMAKE_CXX_FLAGS "{CMAKE_CXX_FLAGS} /fp:fast /Gy /Oi /Oy /O2 /Ot /Zi /EHsc")
        endif()

        ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARINGS)
        if(CMAKE_CL_64)
            set(TARGET_ARCH "x64")
        else(CMAKE_CL_64)
            set(TARGET_ARCH "x86")
        endif(CMAKE_CL_64)
    endif()

    add_compile_options(/wd4819)
elseif(APPLE)
elseif(ANDROID)
endif()

message("target platform : ${CMAKE_SYSTEM_NAME}")

set(SOLUTION_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(EXECUTABLE_OUTPUT_PATH 
    ${SOLUTION_DIR}/bin/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH}
)

set(LIBRARY_OUTPUT_DIRECTORY
    ${SOLUTION_DIR}/bin/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH}
)

link_directories(
    ${SOLUTION_DIR}/lib/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH}
)

include_directories(
    ${SOLUTION_DIR}/include
)

if(MSVC)
    set(THIRD_PART_LIB_DIR ${SOLUTION_DIR}/lib/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH})
    find_library(EGL libEGL ${THIRD_PART_LIB_DIR})
    find_library(GLESv2 libGLESv2 ${THID_PART_LIB_DIR})

    if(${WINDOW_SYSTEM} STREQUAL SDL2)
        find_library(SDL2 SDL2 "SDL2.lib" ${THIRD_PART_LIB_DIR})
    else()
    endif()
endif()



include(CTest)
enable_testing()

# add_executable(LearnDX12 main.cpp)
add_subdirectory(Source)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
