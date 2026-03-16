cmake_minimum_required(VERSION 3.28)
project(Diploma LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SFML_ROOT "Q:/Diploma/vcpkg/installed/x64-mingw-dynamic")
set(VCPKG_ROOT "Q:/Diploma/vcpkg")
set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/x64-mingw-dynamic")
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/share")
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}")

add_library(SFML::System SHARED IMPORTED GLOBAL)
set_target_properties(SFML::System PROPERTIES
        IMPORTED_LOCATION "${SFML_ROOT}/bin/sfml-system-3.dll"
        IMPORTED_IMPLIB "${SFML_ROOT}/lib/libsfml-system.a"
        INTERFACE_INCLUDE_DIRECTORIES "${SFML_ROOT}/include"
)

add_library(SFML::Window SHARED IMPORTED GLOBAL)
set_target_properties(SFML::Window PROPERTIES
        IMPORTED_LOCATION "${SFML_ROOT}/bin/sfml-window-3.dll"
        IMPORTED_IMPLIB "${SFML_ROOT}/lib/libsfml-window.a"
        INTERFACE_INCLUDE_DIRECTORIES "${SFML_ROOT}/include"
        INTERFACE_LINK_LIBRARIES "SFML::System;opengl32;gdi32;winmm"
)

add_library(SFML::Graphics SHARED IMPORTED GLOBAL)
set_target_properties(SFML::Graphics PROPERTIES
        IMPORTED_LOCATION "${SFML_ROOT}/bin/sfml-graphics-3.dll"
        IMPORTED_IMPLIB "${SFML_ROOT}/lib/libsfml-graphics.a"
        INTERFACE_INCLUDE_DIRECTORIES "${SFML_ROOT}/include"
        INTERFACE_LINK_LIBRARIES "SFML::Window;SFML::System"
)

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)

# Исходные файлы
set(SOURCES
        main.cpp
        algorithms/graph/GridGraphBuilder.cpp
        algorithms/graph/VisibilityGraphBuilder.cpp
        algorithms/search/GraphSearch.cpp
        algorithms/planners/GraphBasedPlanners.cpp
        algorithms/planners/RRTPlanner.cpp
        algorithms/planners/PlannerFactory.cpp
        experiment/config/GeneratorConfigIO.cpp
        experiment/generation/SceneBatchGenerator.cpp
        experiment/core/ExperimentService.cpp
        experiment/core/ExperimentIO.cpp
        experiment/metrics/SceneMetricsCalculator.cpp
        experiment/metrics/SceneMetricsBatchCalculator.cpp
        visualization/SceneVisualizer.cpp
        visualization/SceneRenderer.cpp
        visualization/GraphRenderer.cpp
        visualization/UIManager.cpp
        visualization/CameraController.cpp
        serialization/SceneSerializer.cpp
)

set(HEADERS
        include/geometry/Point.h
        include/geometry/Disk.h
        include/geometry/Scene.h
        include/geometry/Path.h
        include/algorithms/Planner.h
        include/algorithms/GraphBuilder.h
        include/algorithms/Graph.h
        include/algorithms/GridGraphBuilder.h
        include/algorithms/VisibilityGraphBuilder.h
        include/algorithms/search/GraphSearch.h
        include/algorithms/GraphBasedPlanners.h
        include/algorithms/RRTPlanner.h
        include/algorithms/PlannerFactory.h
        include/experiment/config/GeneratorConfig.h
        include/experiment/config/GeneratorConfigIO.h
        include/experiment/core/ExperimentModel.h
        include/experiment/generation/SceneBatchGenerator.h
        include/experiment/core/ExperimentService.h
        include/experiment/core/ExperimentIO.h
        include/experiment/metrics/SceneMetricsCalculator.h
        include/experiment/metrics/SceneMetricsBatchCalculator.h
        include/visualization/SceneVisualizer.h
        include/visualization/GraphRenderer.h
        include/serialization/SceneSerializer.h
)

add_executable(Diploma ${SOURCES} ${HEADERS})

target_include_directories(Diploma PRIVATE
        include
        "${SFML_ROOT}/include"
)

add_library(imgui::imgui STATIC IMPORTED GLOBAL)
set_target_properties(imgui::imgui PROPERTIES
        IMPORTED_LOCATION "${VCPKG_INSTALLED_DIR}/lib/libimgui.a"
        INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INSTALLED_DIR}/include"
)

add_library(ImGui-SFML::ImGui-SFML STATIC IMPORTED GLOBAL)
set_target_properties(ImGui-SFML::ImGui-SFML PROPERTIES
        IMPORTED_LOCATION "${VCPKG_INSTALLED_DIR}/lib/libImGui-SFML.a"
        INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INSTALLED_DIR}/include"
        INTERFACE_LINK_LIBRARIES "imgui::imgui;SFML::Graphics;SFML::Window;SFML::System"
)

target_link_libraries(Diploma PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        imgui::imgui
        ImGui-SFML::ImGui-SFML
        nlohmann_json::nlohmann_json
        opengl32
        comdlg32
        gdi32
        winmm
)

add_custom_command(TARGET Diploma POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_ROOT}/bin/sfml-system-3.dll"
        $<TARGET_FILE_DIR:Diploma>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_ROOT}/bin/sfml-window-3.dll"
        $<TARGET_FILE_DIR:Diploma>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_ROOT}/bin/sfml-graphics-3.dll"
        $<TARGET_FILE_DIR:Diploma>
        COMMENT "Copying SFML DLLs"
)

