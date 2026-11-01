
cmake_minimum_required(VERSION 3.10)
project(GardenTowerDefense)

add_subdirectory(external/glfw-3.5.1)
add_subdirectory(external/glm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

add_executable(GardenTowerDefense
    core/main.cpp
    core/Menus.cpp
    world/World.cpp
    src/Glad.cpp
    enemies/Enemies.cpp
)

target_include_directories(GardenTowerDefense PRIVATE
    external/glad/include
    external/learnopengl/include
)

target_link_libraries(GardenTowerDefense PRIVATE
    glfw
    OpenGL::GL
    Freetype::Freetype
    glm::glm
)
