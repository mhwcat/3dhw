cmake_minimum_required(VERSION 3.13)

project(3dhw-dx11 CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_SUPPRESS_REGENERATION True)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

find_package(DirectX11 REQUIRED)

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
    ../common/utils.h
)

set(SOURCE_FILES 

    src/main.cpp)

add_executable(${PROJECT_NAME} WIN32 ${HEADER_FILES} ${SOURCE_FILES})
# Set startup project in VS to actual project instead of "ALL_BUILD"
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../common/)
target_include_directories(${PROJECT_NAME} PUBLIC ${DirectX11_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
target_link_libraries(${PROJECT_NAME} ${DirectX11_LIBRARY})

# Copy assets/ directory containing texture to build dir
file(COPY ${PROJECT_SOURCE_DIR}/../assets DESTINATION ${CMAKE_BINARY_DIR})