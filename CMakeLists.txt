cmake_minimum_required (VERSION 3.14)

project(opengl-renderer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_subdirectory(deps/glm)
#target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/deps/glm)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_ZLIB ON)

add_subdirectory(deps/assimp)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(deps/tinyexr)
if(TARGET tinyexr)
    target_compile_options(tinyexr PRIVATE -w)
endif()

# Find packages
if(NOT EMSCRIPTEN)
    find_package(GLFW3 REQUIRED)
    message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
    find_package(OpenGL REQUIRED)
    add_definitions(${OPENGL_DEFINITIONS})
    find_package(X11 REQUIRED)
endif()

# DSA and version settings
option(USE_GL_DSA "Use OpenGL Direct State Access" OFF)
set(GLSL_VERSION 450)

if (EMSCRIPTEN)
    set(USE_GL_DSA OFF)
    set(GLSL_VERSION 300)
    set(LIBS glm assimp tinyexr)
    #set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
elseif(UNIX)
    set(LIBS ${GLFW3_LIBRARY} X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread freetype assimp glm tinyexr)
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
endif()

# sources
file(GLOB GLAD_SRC ${CMAKE_SOURCE_DIR}/deps/glad/glad.*)
file(GLOB STB_IMAGE_SRC ${CMAKE_SOURCE_DIR}/deps/stb_image/*.*)
file(GLOB IMGUI_SRC
    ${CMAKE_SOURCE_DIR}/deps/imgui/*.h
    ${CMAKE_SOURCE_DIR}/deps/imgui/*.cpp
)
include_directories(${CMAKE_SOURCE_DIR}/deps/imgui)

file(GLOB IMGUI_BACKEND
    ${CMAKE_SOURCE_DIR}/deps/imgui/backends/imgui_impl_glfw.*
    ${CMAKE_SOURCE_DIR}/deps/imgui/backends/imgui_impl_opengl*.*
)

file(GLOB SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/renderer/*.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/*.h
    ${CMAKE_SOURCE_DIR}/src/renderer/renderpass/*.h
    ${CMAKE_SOURCE_DIR}/src/utils/*.h
    ${CMAKE_SOURCE_DIR}/src/gui/*.h
)


add_executable(${PROJECT_NAME} ${SRC_FILES} ${GLAD_SRC} ${STB_IMAGE_SRC} ${IMGUI_SRC} ${IMGUI_BACKEND})
target_link_libraries(${PROJECT_NAME} ${LIBS})
link_directories(${CMAKE_SOURCE_DIR}/lib)

if(EMSCRIPTEN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_SOURCE_DIR="/")
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif()

if(USE_GL_DSA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GL_DSA)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE GLSL_VERSION=${GLSL_VERSION})

if(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

    target_link_options(${PROJECT_NAME} PRIVATE
        "-sDISABLE_EXCEPTION_CATCHING=0"
        "-sUSE_GLFW=3"
        "-sUSE_WEBGL2=1"
        "-sFULL_ES3=1"

        "-sSTACK_SIZE=5242880"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=256MB"
        # shaders and scene models need to be compiled into binary for WASM
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/src/shaders@/src/shaders"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/resources@/resources"
    )
else()

endif()

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/gl)
include_directories(${CMAKE_SOURCE_DIR}/src/gui)
include_directories(${CMAKE_SOURCE_DIR}/src/renderer)
include_directories(${CMAKE_SOURCE_DIR}/src/renderpass)
include_directories(${CMAKE_SOURCE_DIR}/utils)
include_directories(${CMAKE_SOURCE_DIR}/deps/)
include_directories(${CMAKE_SOURCE_DIR}/deps/assimp/include)
include_directories(${CMAKE_SOURCE_DIR}/deps/stb_image)
include_directories(${CMAKE_SOURCE_DIR}/deps/tinyexr)
include_directories(${CMAKE_SOURCE_DIR}/deps/tinyexr/deps/miniz)
include_directories(${CMAKE_SOURCE_DIR}/deps/assimp/include)
