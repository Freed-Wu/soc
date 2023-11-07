enable_testing()
find_package(GTest)
if(GTest_FOUND)
  add_executable(transmission_protocol_test transmission_protocol_test.cc)
  target_link_libraries(
    transmission_protocol_test ${GTEST_MAIN_LIBRARIES} transmission_protocol)

  include(GoogleTest)
  gtest_discover_tests(transmission_protocol_test)
endif()
