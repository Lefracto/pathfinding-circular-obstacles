cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "vcpkg target triplet")
endif()

set(VCPKG_APPLOCAL_DEPS OFF CACHE BOOL "Use explicit project runtime DLL copying instead" FORCE)
option(DIPLOMA_STATIC_BUNDLE "Build a mostly self-contained executable with static vcpkg/MinGW runtime libraries" OFF)

if(VCPKG_TARGET_TRIPLET MATCHES "static")
        set(DIPLOMA_STATIC_BUNDLE ON CACHE BOOL "Build a mostly self-contained executable with static vcpkg/MinGW runtime libraries" FORCE)
endif()

if(CMAKE_HOST_WIN32 AND DEFINED CMAKE_CXX_COMPILER AND NOT DEFINED CMAKE_RC_COMPILER)
        get_filename_component(_DIPLOMA_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        find_program(_DIPLOMA_WINDRES NAMES windres x86_64-w64-mingw32-windres HINTS "${_DIPLOMA_COMPILER_DIR}" NO_DEFAULT_PATH)
        if(_DIPLOMA_WINDRES)
                set(CMAKE_RC_COMPILER "${_DIPLOMA_WINDRES}" CACHE FILEPATH "Windows resource compiler")
        endif()
endif()

project(Diploma VERSION 1.2.15 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VCPKG_ROOT "Q:/Diploma/vcpkg" CACHE PATH "vcpkg root directory")
set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
set(VCPKG_BIN_DIR "${VCPKG_INSTALLED_DIR}/bin")
get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(DIPLOMA_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(DIPLOMA_APP_DISPLAY_NAME "Circular Obstacle Pathfinding Lab" CACHE STRING "Human-readable application name" FORCE)
set(DIPLOMA_APP_EXECUTABLE_BASENAME "CircularObstaclePathfindingLab" CACHE STRING "Base name for generated executable files" FORCE)
set(DIPLOMA_APP_ICON "${DIPLOMA_REPO_ROOT}/app.ico" CACHE FILEPATH "Windows application icon")
set(DIPLOMA_APP_ICON_SVG "${DIPLOMA_REPO_ROOT}/app.svg" CACHE FILEPATH "Runtime SVG application icon")
set(DIPLOMA_DIST_DIR "${DIPLOMA_REPO_ROOT}/dist" CACHE PATH "Distribution output directory")
set(DIPLOMA_WORKSPACE_WINDOW_TITLE "${DIPLOMA_APP_DISPLAY_NAME} v${PROJECT_VERSION}")
set(DIPLOMA_SCENE_WINDOW_TITLE "${DIPLOMA_APP_DISPLAY_NAME} - Scene View")
set(DIPLOMA_EXECUTABLE_NAME "${DIPLOMA_APP_EXECUTABLE_BASENAME}-v${PROJECT_VERSION}")
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/share")
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}")

if(DIPLOMA_STATIC_BUNDLE)
        set(SFML_STATIC_LIBRARIES ON)
        add_link_options(-static -static-libgcc -static-libstdc++)
else()
        configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_runtime_dlls.configured.ps1.in"
                "${CMAKE_CURRENT_BINARY_DIR}/copy_runtime_dlls.ps1"
                @ONLY
        )
endif()

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/link_with_mingw_path.cmd.in"
        "${CMAKE_CURRENT_BINARY_DIR}/link_with_mingw_path.cmd"
        @ONLY
)

if(WIN32)
        configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/app.rc.in"
                "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
                @ONLY
        )
endif()

find_package(SFML 3 COMPONENTS Graphics Window System CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
find_package(NanoSVG CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)

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
        visualization/AppAssets.cpp
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
        include/algorithms/GraphSearch.h
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
        include/visualization/AppInfo.h
        include/visualization/AppAssets.h
        include/serialization/SceneSerializer.h
)

set(WINDOWS_RESOURCES)
if(WIN32)
        list(APPEND WINDOWS_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
endif()

add_executable(Diploma ${SOURCES} ${HEADERS} ${WINDOWS_RESOURCES})
set_property(TARGET Diploma PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_CURRENT_BINARY_DIR}/link_with_mingw_path.cmd")
set_property(TARGET Diploma PROPERTY RULE_LAUNCH_LINK "${CMAKE_CURRENT_BINARY_DIR}/link_with_mingw_path.cmd")
set_target_properties(Diploma PROPERTIES OUTPUT_NAME "${DIPLOMA_EXECUTABLE_NAME}")

target_compile_definitions(Diploma PRIVATE
        DIPLOMA_PROJECT_VERSION="${PROJECT_VERSION}"
        DIPLOMA_APP_DISPLAY_NAME="${DIPLOMA_APP_DISPLAY_NAME}"
        DIPLOMA_WORKSPACE_WINDOW_TITLE="${DIPLOMA_WORKSPACE_WINDOW_TITLE}"
        DIPLOMA_SCENE_WINDOW_TITLE="${DIPLOMA_SCENE_WINDOW_TITLE}"
)

target_include_directories(Diploma PRIVATE
        include
)

target_link_libraries(Diploma PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        imgui::imgui
        ImGui-SFML::ImGui-SFML
        NanoSVG::nanosvgrast
        nlohmann_json::nlohmann_json
        opengl32
        comdlg32
        gdi32
        winmm
)

if(NOT DIPLOMA_STATIC_BUNDLE)
        add_custom_command(TARGET Diploma POST_BUILD
                COMMAND powershell.exe
                -NoProfile
                -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_BINARY_DIR}/copy_runtime_dlls.ps1"
                $<TARGET_FILE_DIR:Diploma>
                COMMENT "Copying runtime DLLs"
                VERBATIM
        )
endif()

if(DIPLOMA_STATIC_BUNDLE)
        add_custom_command(TARGET Diploma POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${DIPLOMA_DIST_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Diploma>"
                "${DIPLOMA_DIST_DIR}/$<TARGET_FILE_NAME:Diploma>"
                COMMENT "Copying static release package to dist"
                VERBATIM
        )

        add_custom_target(DiplomaDist
                COMMAND ${CMAKE_COMMAND} -E make_directory "${DIPLOMA_DIST_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Diploma>"
                "${DIPLOMA_DIST_DIR}/$<TARGET_FILE_NAME:Diploma>"
                DEPENDS Diploma
                COMMENT "Copying static release package to dist"
                VERBATIM
        )
endif()

