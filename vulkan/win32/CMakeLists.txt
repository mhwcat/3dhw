cmake_minimum_required(VERSION 3.13)

project(3dhw-vulkan C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_SUPPRESS_REGENERATION True)

# Try to find Vulkan SDK based on "VULKAN_SDK" environment variable, show error if not found
if(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_DIR $ENV{VULKAN_SDK} CACHE PATH "LunarG Vulkan SDK path")
else()
    set(VULKAN_SDK_DIR "VULKAN-NOTFOUND" CACHE PATH "LunarG Vulkan SDK path")
endif()
if(VULKAN_SDK_DIR STREQUAL "VULKAN-NOTFOUND")
    message(FATAL_ERROR "LunarG Vulkan SDK not found, please provide path manually to VULKAN_SDK_DIR variable.")
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if (MSVC)
    #add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused)
endif()

# In order for headers to appear in Visual Studio, we have to add them as sources
set(HEADER_FILES
    ../../common/cube.h
    ../../common/utils.h
    ../include/vkdebug.h
    ../include/shader.h
    ../include/swapchain.h
    ../include/device.h
    ../include/pipeline.h
    ../include/vkdata.h
    ../include/buffers.h
    ../include/texture.h
)

set(SOURCE_FILES 
    ../src/vkdebug.c
    ../src/shader.c
    ../src/swapchain.c
    ../src/device.c
    ../src/pipeline.c
    ../src/vkdata.c
    ../src/buffers.c
    ../src/texture.c
    src/main.c)

add_executable(${PROJECT_NAME} WIN32 ${HEADER_FILES} ${SOURCE_FILES})
# Set startup project in VS to actual project instead of "ALL_BUILD"
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../include/)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../../common/)
target_include_directories(${PROJECT_NAME} PUBLIC ${VULKAN_SDK_DIR}/include/)

target_link_directories(${PROJECT_NAME} PUBLIC ${VULKAN_SDK_DIR}/lib)

target_link_libraries(${PROJECT_NAME} vulkan-1)   # link Vulkan lib
target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})

# Copy assets/ directory containing texture to build dir
file(COPY ${PROJECT_SOURCE_DIR}/../../assets DESTINATION ${CMAKE_BINARY_DIR})