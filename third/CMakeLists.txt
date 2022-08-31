cmake_minimum_required(VERSION 3.6)
project(neodb-third-party)

INCLUDE(ExternalProject)

IF(TARGET libzbd)
  MESSAGE("[neodb] libzbd target exist, skip build")
ELSE()
  MESSAGE("[neodb] libzbd not found, building...")
  ExternalProject_Add(libzbd-project
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libzbd
    CONFIGURE_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/libzbd && sh ./autogen.sh && ./configure --prefix=${CMAKE_BINARY_DIR}
    BUILD_IN_SOURCE 0
    BUILD_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/libzbd && ${MAKE}
    INSTALL_COMMAND ${MAKE} install
    )
  ADD_LIBRARY(libzbd STATIC IMPORTED GLOBAL)
  ADD_DEPENDENCIES(libzbd libzbd-project)
  SET_TARGET_PROPERTIES(libzbd PROPERTIES
    IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libzbd.a
    INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/include
    POSITION_INDEPENDENT_CODE ON
  )
ENDIF()
