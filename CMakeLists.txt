# CMakeList.txt : CMake project for water_opengl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("magnum_starter" LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(spdlog CONFIG REQUIRED)
find_package(Corrade REQUIRED Main Containers)
find_package(Magnum REQUIRED GL Shaders MeshTools Primitives Trade)
find_package(MagnumIntegration REQUIRED ImGui Glm)

if(CORRADE_TARGET_ANDROID)
    find_package(Magnum REQUIRED AndroidApplication)
elseif(CORRADE_TARGET_EMSCRIPTEN)
    find_package(Magnum REQUIRED EmscriptenApplication)
else()
    find_package(Magnum REQUIRED Sdl2Application)
endif()

set(VCPKG_DEPS 
    spdlog::spdlog
    Corrade::Main
    Magnum::Application
    Corrade::Containers
    Magnum::GL
    Magnum::Magnum
    Magnum::Shaders
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::Trade
    MagnumIntegration::ImGui
    MagnumIntegration::Glm)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

#file(GLOB_RECURSE FAST_NOISE_SIMD 
#"source/engine/FastNoiseSIMD/*.cpp" 
#"source/engine/FastNoiseSIMD/*.h")
#message(WARNING ${FAST_NOISE_SIMD})

add_library(engine
 "source/engine/object_pool.h" 
 "source/engine/object_pool.cpp"
 "source/engine/overlay.cpp"
 "source/engine/overlay.h"
 "source/engine/camera.h"
 "source/engine/free_camera.h" 
 "source/engine/free_camera.cpp"
 "source/engine/debug_draw.h"
 "source/engine/debug_draw.cpp"
 "source/engine/ImGuizmo.h"
 "source/engine/ImGuizmo.cpp"
 "source/engine/fast_noise.h")

target_link_libraries(engine PUBLIC ${VCPKG_DEPS})

# Add source to this project's executable.
add_executable (cube WIN32 "source/cube.main.cpp")
target_link_libraries(cube PUBLIC engine Corrade::Main)

# Add source to this project's executable.
corrade_add_resource(Shader_RESOURCES "source/shader/resources.conf")
add_executable (terrain "source/terrain.main.cpp"  ${Shader_RESOURCES})
target_link_libraries(terrain PUBLIC engine Corrade::Main)

# TODO: Add tests and install targets if needed.
