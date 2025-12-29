cmake_minimum_required(VERSION 3.28)
project(Diploma LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ВАЖНО: Отключите toolchain файл vcpkg для этой конфигурации
# Удалите из CLion Settings -> CMake options строку с -DCMAKE_TOOLCHAIN_FILE

# Ручное указание путей к SFML (из вашего установленного vcpkg)
set(SFML_ROOT "Q:/Diploma/vcpkg/installed/x64-mingw-dynamic")

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

# Исходные файлы
set(SOURCES
        main.cpp
        algorithms/graph/GridGraphBuilder.cpp
        visualization/SceneVisualizer.cpp
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
        include/visualization/SceneVisualizer.h
)

add_executable(Diploma ${SOURCES} ${HEADERS})

# Включаем директории
target_include_directories(Diploma PRIVATE
        include
        "${SFML_ROOT}/include"
)

# Связываем с SFML и необходимыми системными библиотеками
target_link_libraries(Diploma PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
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