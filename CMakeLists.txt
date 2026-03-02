cmake_minimum_required (VERSION 3.14)

project(opengl-model-viewer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

link_directories(${CMAKE_SOURCE_DIR}/lib)

add_subdirectory(deps/glm)

# find the required packages
find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")
find_package(ASSIMP REQUIRED)
message(STATUS "Found ASSIMP in ${ASSIMP_INCLUDE_DIR}")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
find_package(OpenGL REQUIRED)
add_definitions(${OPENGL_DEFINITIONS})
find_package(X11 REQUIRED)

#add_compile_definitions(STB_IMAGE_IMPLEMENTATION)

# note that the order is important for setting the libs
# use pkg-config --libs $(pkg-config --print-requires --print-requires-private glfw3) in a terminal to confirm
set(LIBS ${GLFW3_LIBRARY} X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread freetype ${ASSIMP_LIBRARY} glm)
set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")

# sources
file(GLOB GLAD_SRC ${CMAKE_SOURCE_DIR}/deps/glad/glad.*)
file(GLOB IMGUI_SRC 
    ${CMAKE_SOURCE_DIR}/deps/imgui/*.h
    ${CMAKE_SOURCE_DIR}/deps/imgui/*.cpp
)
include_directories(${CMAKE_SOURCE_DIR}/deps/imgui)

file(GLOB IMGUI_BACKEND
    ${CMAKE_SOURCE_DIR}/deps/imgui/backends/imgui_impl_glfw.*
    ${CMAKE_SOURCE_DIR}/deps/imgui/backends/imgui_impl_opengl*.*
)
#include_directories(${CMAKE_SOURCE_DIR}/deps/imgui)


file(GLOB SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/renderer/*.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/*.h
)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/gui)
include_directories(${CMAKE_SOURCE_DIR}/renderer)
include_directories(${CMAKE_SOURCE_DIR}/deps/)
add_executable(${PROJECT_NAME} ${SRC_FILES} ${GLAD_SRC} ${IMGUI_SRC} ${IMGUI_BACKEND})
target_link_libraries(${PROJECT_NAME} ${LIBS})



#set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}")

file(GLOB SHADERS
    "src/shaders/*.vert"
    "src/shaders/*.frag"
    "src/shaders/*.geom"
    "src/shaders/*.comp"
    "src/shaders/*.tcs"
    "src/shaders/*.tes"
)
file(GLOB DLLS "dlls/*.dll")

foreach(SHADER ${SHADERS})
    file(COPY ${SHADER} DESTINATION ${CMAKE_BINARY_DIR})
endforeach(SHADER)