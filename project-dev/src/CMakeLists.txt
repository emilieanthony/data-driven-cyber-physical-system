cmake_minimum_required(VERSION 3.2)
project(template-opencv)
set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/template-opencv.cpp)

# We should add testing here
#enable_testing()
# add_executable(${PROJECT_NAME}-Runner TestPrimeChecker.cpp ${CMAKE_CURRENT_SOURCE_DIR}/PrimeChecker.cpp)
# addadd_test(NAME ${PROJECT_NAME}-Runner COMMAND ${PROJECT_NAME}-Runner)_test(NAME ${PROJECT_NAME}-Runner COMMAND ${PROJECT_NAME}-Runner)