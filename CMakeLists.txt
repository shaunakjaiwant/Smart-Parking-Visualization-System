cmake_minimum_required(VERSION 3.16)
project(SmartParking VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure build options
option(BUILD_TESTS "Build automated unit and integration tests" ON)

# External dependencies
# 1. GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# 2. GLAD
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# 3. Dear ImGui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
)
target_link_libraries(imgui PUBLIC glfw)

# Project Core Library
set(SMART_PARKING_CORE_SOURCES
    src/graphics/Shader.cpp
    src/graphics/Camera.cpp
    src/graphics/Mesh.cpp
    src/graphics/ShadowMap.cpp
    src/graphics/Renderer.cpp
    src/parking/ParkingSlot.cpp
    src/parking/ParkingLot.cpp
    src/vehicle/Vehicle.cpp
    src/navigation/Graph.cpp
    src/navigation/PathFinder.cpp
    src/simulation/Simulation.cpp
    src/ui/UIManager.cpp
    src/application/Application.cpp
)

set(SMART_PARKING_CORE_HEADERS
    src/graphics/Shader.h
    src/graphics/Camera.h
    src/graphics/Mesh.h
    src/graphics/ShadowMap.h
    src/graphics/Renderer.h
    src/parking/ParkingSlot.h
    src/parking/ParkingLot.h
    src/vehicle/Vehicle.h
    src/navigation/Graph.h
    src/navigation/PathFinder.h
    src/simulation/Simulation.h
    src/ui/UIManager.h
    src/application/Application.h
)

add_library(SmartParkingCore STATIC
    ${SMART_PARKING_CORE_SOURCES}
    ${SMART_PARKING_CORE_HEADERS}
)

target_include_directories(SmartParkingCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

target_link_libraries(SmartParkingCore PUBLIC
    glad
    glfw
    imgui
)

if (WIN32)
    target_link_libraries(SmartParkingCore PUBLIC opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(SmartParkingCore PUBLIC OpenGL::GL)
endif()

# Main Application Executable
add_executable(SmartParking src/main.cpp)
target_link_libraries(SmartParking PRIVATE SmartParkingCore)

# Copy shaders and config into runtime directory
add_custom_command(TARGET SmartParking POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:SmartParking>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        $<TARGET_FILE_DIR:SmartParking>/config
)

# Automated Test Executable
if (BUILD_TESTS)
    enable_testing()
    add_executable(SmartParkingTests tests/test_main.cpp)
    target_link_libraries(SmartParkingTests PRIVATE SmartParkingCore)
    add_test(NAME SmartParkingUnitTests COMMAND SmartParkingTests)
    add_custom_command(TARGET SmartParkingTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/config
            $<TARGET_FILE_DIR:SmartParkingTests>/config
    )
endif()
