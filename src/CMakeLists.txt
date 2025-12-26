cmake_minimum_required(VERSION 3.28)
project(Diploma)

set(CMAKE_CXX_STANDARD 20)

add_executable(Diploma main.cpp
        include/geometry/Point.h
        include/geometry/Disk.h
        include/geometry/Disk.h
        include/geometry/Scene.h
        include/algorithms/Planner.h
        include/geometry/Path.h
        include/algorithms/GraphBuilder.h
        include/algorithms/Graph.h
        algorithms/graph/GridGraphBuilder.cpp
)
