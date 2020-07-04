cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(RapidjsonWrapper LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

#default
find_package(Threads REQUIRED)
find_package(RapidJSON REQUIRED)

include(ExternalProject)
externalproject_add(
  googletest 
  URL https://github.com/google/googletest/archive/release-1.10.0.zip
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/lib
  INSTALL_COMMAND ""
)

ExternalProject_Get_Property(googletest binary_dir)
ExternalProject_Get_Property(googletest source_dir)
add_library(gtest UNKNOWN IMPORTED)
add_library(gtest_main UNKNOWN IMPORTED)
set_target_properties(gtest PROPERTIES
    IMPORTED_LOCATION ${binary_dir}/libgtest.a
)
set_target_properties(gtest_main PROPERTIES
    IMPORTED_LOCATION ${binary_dir}/libgtest_main.a
)
add_dependencies(gtest googletest)
add_dependencies(gtest_main googletest)

set(GTestInclude ${source_dir}/googletest/include)
message(STATUS "gtest include : ${GTestInclude}")

include_directories(${CMAKE_SOURCE_DIR} ${GTestInclude} ${Rapidjson_INCLUDE_DIRS})

add_executable(json_test ${CMAKE_CURRENT_SOURCE_DIR}/test/json_unittest.cpp)


include(cmake/TargetDistclean.cmake)

######################################
# Configure the test to use GoogleTest
#
# If used often, could be made a macro.

add_dependencies(json_test gtest)
target_link_libraries(json_test gtest)
target_link_libraries(json_test gtest_main)

##################################
# Just make the test runnable with
#   $ make test

enable_testing()
add_test(NAME    json_test 
         COMMAND json_test)
