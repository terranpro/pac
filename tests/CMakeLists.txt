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

set( callback_test_srcs
  callback-test.cpp
  )

add_executable( callback-test
  ${callback_test_srcs}
  )

include_directories(
  ${CMAKE_SOURCE_DIR}/lib
  )

set( context_test_srcs
  context-test.cpp
  )

add_executable( context-test
  ${context_test_srcs}
  )

include_directories(
  ${CMAKE_SOURCE_DIR}/lib
  )

