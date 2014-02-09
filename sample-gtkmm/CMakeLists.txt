project( sample-gtkmm )

find_package(PkgConfig)
pkg_check_modules( GTKMM gtkmm-3.0 )

file(
  GLOB_RECURSE
  prj_srcs
  *.cpp
  )

message( "${prj_srcs}" )

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -g ")

add_executable(
  sample-gtkmm
  ${prj_srcs}
  )

include_directories(
  include
  ${GTKMM_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/lib
  )

link_directories(
  ${GTKMM_LIBRARY_DIRS}
  )

target_link_libraries(
  sample-gtkmm
  ${GTKMM_LIBRARIES}
  )
