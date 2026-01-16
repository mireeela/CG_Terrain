cmake_minimum_required(VERSION 3.20)
project(CG_Terrain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    .
    external/glad/include
    external/glm
    external/glfw/include
)

file(GLOB SOURCES
    "*.cpp"
    "*.c"
    "external/glad/src/*.c"
)

add_executable(${PROJECT_NAME} ${SOURCES})

add_subdirectory(external/glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
elseif(APPLE)
    find_library(OPENGL OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL})
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL dl pthread X11)
endif()
