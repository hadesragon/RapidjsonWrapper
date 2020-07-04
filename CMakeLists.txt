cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(RapidjsonWrapper LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

# Default FindPackage
find_package(Threads REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(GTest REQUIRED)

# include Cmake Module
include(cmake/TargetDistclean.cmake)

# include
set(GTestInclude ${source_dir}/googletest/include)
set(OptionalInclude ${CMAKE_SOURCE_DIR}/optional-lite/include)
set(StringViewInclude ${CMAKE_SOURCE_DIR}/string-view-lite/include)
include_directories(${CMAKE_SOURCE_DIR} ${GTestInclude} ${RAPIDJSON_INCLUDE_DIRS} ${OptionalInclude} ${StringViewInclude})


# excutable
add_executable(json_test ${CMAKE_CURRENT_SOURCE_DIR}/test/json_unittest.cpp)


######################################
# Configure the test to use GoogleTest
#
# If used often, could be made a macro.

target_link_libraries(json_test GTest::GTest GTest::Main)

##################################
# Just make the test runnable with
#   $ make test

enable_testing()
add_test(NAME    json_test 
         COMMAND json_test)
