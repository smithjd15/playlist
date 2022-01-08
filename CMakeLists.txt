cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(playlist)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "-O2")

option(LIBCURL "Verify links with libcurl" ON)
option(TAGLIB "Populate playlist metadata with taglib" ON)

find_package(pugixml REQUIRED)
find_package(RapidJSON REQUIRED)

add_executable(playlist
               src/main.cpp
               src/playlist.cpp
               src/jspf.cpp
               src/m3u.cpp
               src/pls.cpp
               src/xspf.cpp)

target_link_libraries(playlist pugixml RapidJSON)

if(LIBCURL)
  find_package(CURL)
  target_link_libraries(playlist curl)
  add_definitions(-DLIBCURL)
endif()

if(TAGLIB)
  find_package(TagLib)
  target_link_libraries(playlist tag)
  add_definitions(-DTAGLIB)
endif()

install(TARGETS playlist RUNTIME DESTINATION /usr/bin)
