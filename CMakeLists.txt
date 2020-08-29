# CMakeList.txt : CMake project for water_opengl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("water_opengl" LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(spdlog CONFIG REQUIRED)
find_package(Corrade REQUIRED Main)
find_package(Magnum REQUIRED GL Shaders Sdl2Application)
find_package(MagnumIntegration REQUIRED ImGui Glm)


set(VCPKG_DEPS 
    spdlog::spdlog     
    Corrade::Main
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::Shaders
    MagnumIntegration::ImGui
    MagnumIntegration::Glm)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

# Add source to this project's executable.
add_executable (water_opengl  source/test.main.cpp)
target_link_libraries(water_opengl PRIVATE ${VCPKG_DEPS})

# TODO: Add tests and install targets if needed.
