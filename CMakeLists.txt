cmake_minimum_required(VERSION 3.16)
project(week09_camera CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)

# GLM
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)

FetchContent_MakeAvailable(glfw glm)

# GLAD: tools/glad/include/glad/glad.h және tools/glad/src/glad.c болуы керек
add_executable(week09 main.cpp tools/glad/src/glad.c)
target_include_directories(week09 PRIVATE tools/glad/include)
target_link_libraries(week09 PRIVATE glfw glm::glm)
