project( pac-tests )

macro( pac_test TEST_SRC_FILE )
  message( "Creating Test: ${TEST_SRC_FILE}" )
  string( REGEX REPLACE ".cpp" "" TEST_SRC_NAME ${TEST_SRC_FILE} )
  add_executable(
    ${TEST_SRC_NAME}
    ${TEST_SRC_FILE}
    )
endmacro( pac_test )

include_directories(
  ${CMAKE_SOURCE_DIR}/lib
  )

set( CMAKE_CXX_FLAGS
  ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread -g -gdwarf-2"
  )

pac_test( signal-test.cpp )
pac_test( callback-test.cpp )
pac_test( context-test.cpp )
