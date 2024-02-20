cmake_minimum_required(VERSION 3.15)
project(neodb-third-parties)
INCLUDE(ExternalProject)

# gtest
if (NOT TARGET gtest)
    ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/googletest)
endif ()

if (NOT TARGET spdlog)
    ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/spdlog)
endif ()
