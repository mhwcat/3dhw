cmake_minimum_required(VERSION 3.6)

project(3dhw-opengl C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_SUPPRESS_REGENERATION True)
set(VS_STARTUP_PROJECT ${PROJECT_NAME})

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if (MSVC)
#    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused)
endif()

# In order for headers to appear in Visual Studio, we have to add them as sources
set(HEADER_FILES
    ../../common/cube.h
    ../../common/utils.h
    ../include/gldebug.h
    ../include/mesh.h
    ../include/shader.h
    ../include/texture.h
)

set(SOURCE_FILES 
    ../../common/glad/src/wgl.c 
    ../../common/glad/src/gl.c 
    ../src/mesh.c 
    ../src/shader.c 
    ../src/texture.c 
    src/main.c)

add_executable(${PROJECT_NAME} WIN32 ${HEADER_FILES} ${SOURCE_FILES})
# Set startup project in VS to actual project instead of "ALL_BUILD"
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../include/)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../../common/glad/include/)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../../common/)

target_link_libraries(${PROJECT_NAME} opengl32)
target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})

# Copy assets/ directory containing texture to build dir
file(COPY ${PROJECT_SOURCE_DIR}/../../assets DESTINATION ${CMAKE_BINARY_DIR})