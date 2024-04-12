CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(neodb LANGUAGES C CXX)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

# options
OPTION(WITH_ASAN "build with asan" OFF)
OPTION(WITH_TEST "build test cases" ON)

ADD_SUBDIRECTORY(third)

IF (WITH_ASAN)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
ENDIF ()

# header files
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src)

# source files
FILE(GLOB_RECURSE SOURCE_FILES src/*.cc)
FILE(GLOB_RECURSE TEST_FILES src/*_test.cc)
LIST(REMOVE_ITEM SOURCE_FILES ${TEST_FILES})

# build neodb library
ADD_LIBRARY(neodb STATIC ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(neodb spdlog gflags rt)

# build neodb test cases
IF (WITH_TEST)
    INCLUDE(CTest)
    ENABLE_TESTING()
    FOREACH (test_file ${TEST_FILES})
        GET_FILENAME_COMPONENT(exename ${test_file} NAME_WE)
        ADD_EXECUTABLE(${exename} ${ARTIFACT_SUFFIX} ${test_file})
        MESSAGE("build tests: ${exename}${ARTIFACT_SUFFIX}")
        SET_TARGET_PROPERTIES(${exename} ${ARTIFACT_SUFFIX}
                PROPERTIES OUTPUT_NAME ${exename} ${ARTIFACT_SUFFIX}
        )
        TARGET_LINK_LIBRARIES(${exename} ${ARTIFACT_SUFFIX} gtest gtest_main neodb)
        ADD_TEST(NAME ${exename} COMMAND ${exename})
    ENDFOREACH ()
ENDIF ()

# build tools
ADD_EXECUTABLE(neodb_bench tools/neodb_bench.cc)
TARGET_LINK_LIBRARIES(neodb_bench neodb)