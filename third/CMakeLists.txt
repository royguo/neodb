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

if (NOT TARGET gflags)
    ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/gflags)
endif ()

#if (NOT TARGET oneTBB)
#    ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/oneTBB)
#endif ()

#if (NOT TARGET folly)
#    ExternalProject_Add(folly-project
#            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/folly
#            CONFIGURE_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/folly && python3 ./build/fbcode_builder/getdeps.py --allow-system-packages build
#            BUILD_IN_SOURCE 1
#            BUILD_COMMAND ""
#            INSTALL_COMMAND ""
#    )
#    ADD_LIBRARY(folly STATIC IMPORTED GLOBAL)
#    ADD_DEPENDENCIES(folly folly-project)
#    SET_TARGET_PROPERTIES(folly PROPERTIES
#            IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libfolly.a
#            INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/include
#            POSITION_INDEPENDENT_CODE ON
#    )
#endif ()