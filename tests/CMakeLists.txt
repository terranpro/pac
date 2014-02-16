project( pac-tests )

set( test_srcs
  signal-test.cpp
  )

set( CMAKE_CXX_FLAGS
  ${CMAKE_CXX_FLAGS} "-std=c++11"
  )

add_executable( signal-test
  ${test_srcs}
  )

include_directories(
  ${CMAKE_SOURCE_DIR}/lib
  )

target_link_libraries(
  sample-gtkmm
  )
