cmake_minimum_required (VERSION 3.1)

project (ScrybeIO)

# Setting the path to output folder where executables are created
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

include_directories("${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/unit-tests"
  ${GTEST_INCLUDE_DIRS}
)

add_executable(scrybeio-test 
  "${PROJECT_SOURCE_DIR}/src/server_test.cpp"
  "${PROJECT_SOURCE_DIR}/src/Device.cpp"
  "${PROJECT_SOURCE_DIR}/src/Worker.cpp"
  "${PROJECT_SOURCE_DIR}/src/Options.cpp"
)

# CMake compilation and linking with -pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(scrybeio-test Threads::Threads)

# GTest related
find_package(GTest REQUIRED)
add_executable(runTests 
  "${PROJECT_SOURCE_DIR}/src/Device.cpp"
  "${PROJECT_SOURCE_DIR}/src/Worker.cpp"
  "${PROJECT_SOURCE_DIR}/src/Options.cpp"
  "${PROJECT_SOURCE_DIR}/unit-tests/Example_Test.cpp"
)

target_link_libraries(runTests 
  ${GTEST_LIBRARIES} 
  Threads::Threads
)

