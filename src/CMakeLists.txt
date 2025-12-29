cmake_minimum_required(VERSION 3.28)
project(Diploma LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ВАЖНО: Отключите toolchain файл vcpkg для этой конфигурации
# Удалите из CLion Settings -> CMake options строку с -DCMAKE_TOOLCHAIN_FILE

# Ручное указание путей к SFML (из вашего установленного vcpkg)
set(SFML_ROOT "Q:/Diploma/vcpkg/installed/x64-mingw-dynamic")
set(VCPKG_ROOT "Q:/Diploma/vcpkg")
set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/x64-mingw-dynamic")
# Добавляем пути для поиска пакетов
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/share")
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}")

# Создаем импортированные цели для SFML
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

# nlohmann-json via FetchContent (header-only library)
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
        visualization/SceneVisualizer.cpp
        visualization/SceneRenderer.cpp
        visualization/GraphRenderer.cpp
        visualization/UIManager.cpp
        visualization/CameraController.cpp
        serialization/SceneSerializer.cpp
)

# Заголовочные файлы
set(HEADERS
        include/geometry/Point.h
        include/geometry/Disk.h
        include/geometry/Scene.h
        include/geometry/Path.h
        include/geometry/RandomObstacleGenerator.h
        include/geometry/NaiveObstacleSampler.h
        include/algorithms/Planner.h
        include/algorithms/GraphBuilder.h
        include/algorithms/Graph.h
        include/algorithms/GridGraphBuilder.h
        include/algorithms/VisibilityGraphBuilder.h
        include/visualization/SceneVisualizer.h
        include/visualization/GraphRenderer.h
        include/serialization/SceneSerializer.h
)

add_executable(Diploma ${SOURCES} ${HEADERS})

# Включаем директории
target_include_directories(Diploma PRIVATE
        include
        "${SFML_ROOT}/include"
)

# Подключаем ImGui вручную (как SFML)
add_library(imgui::imgui STATIC IMPORTED GLOBAL)
set_target_properties(imgui::imgui PROPERTIES
        IMPORTED_LOCATION "${VCPKG_INSTALLED_DIR}/lib/libimgui.a"
        INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INSTALLED_DIR}/include"
)

# Подключаем ImGui-SFML вручную
add_library(ImGui-SFML::ImGui-SFML STATIC IMPORTED GLOBAL)
set_target_properties(ImGui-SFML::ImGui-SFML PROPERTIES
        IMPORTED_LOCATION "${VCPKG_INSTALLED_DIR}/lib/libImGui-SFML.a"
        INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INSTALLED_DIR}/include"
        INTERFACE_LINK_LIBRARIES "imgui::imgui;SFML::Graphics;SFML::Window;SFML::System"
)

# Связываем с SFML и необходимыми системными библиотеками
target_link_libraries(Diploma PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        imgui::imgui
        ImGui-SFML::ImGui-SFML
        nlohmann_json::nlohmann_json
        opengl32
        gdi32
        winmm
)

# Копирование DLL
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