cmake_minimum_required(VERSION 3.0)

project(playlist)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "-O2")

option(LIBCURL "Verify links with libcurl" ON)

find_package(pugixml REQUIRED)

add_executable(playlist main.cpp)

target_link_libraries(playlist pugixml)

if(LIBCURL)
  find_package(CURL)
  target_link_libraries(playlist curl)
  add_definitions(-DLIBCURL)
endif()

install(TARGETS playlist RUNTIME DESTINATION /usr/bin)
