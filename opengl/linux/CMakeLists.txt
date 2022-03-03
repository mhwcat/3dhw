cmake_minimum_required(VERSION 3.4)

project(3dhw-opengl C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_SUPPRESS_REGENERATION True)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused)
endif()

set(SOURCE_FILES 
    ../../common/glad/src/gl.c 
    ../../common//glad/src/glx.c 
    ../src/mesh.c
    ../src/shader.c
    ../src/texture.c
    src/main.c)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../include/)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../../common/glad/include/)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../../common/)

target_link_libraries(${PROJECT_NAME} X11)  # link X11 lib
target_link_libraries(${PROJECT_NAME} GL)   # link GL lib
target_link_libraries(${PROJECT_NAME} m)    # link math lib
target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})

# Copy assets/ directory containing texture to build dir
file(COPY ${PROJECT_SOURCE_DIR}/../../assets DESTINATION ${CMAKE_BINARY_DIR})