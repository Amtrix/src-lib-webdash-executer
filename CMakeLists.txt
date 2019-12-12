cmake_minimum_required(VERSION 3.10)

FIND_PACKAGE(Boost 1.65 COMPONENTS system filesystem REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
INCLUDE_DIRECTORIES( "$ENV{MYWORLD}/src/common" )
INCLUDE_DIRECTORIES( "$ENV{MYWORLD}/src/bin/_webdash/common" )
INCLUDE_DIRECTORIES( "${CMAKE_CURRENT_SOURCE_DIR}/include" )

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "$ENV{MYWORLD}/app-persistent/lib")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "$ENV{MYWORLD}/app-persistent/lib")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "$ENV{MYWORLD}/app-persistent/lib")

set (EXTERNAL_LIB_PATH "$ENV{MYWORLD}/src/lib/external")
set (CMAKE_CXX_COMPILER /usr/bin/g++-9)
set (CMAKE_CXX_FLAGS "-std=c++1z -msse4.2 -Wall -Wextra -O3 -g -fopenmp -lstdc++fs")

include_directories(${EXTERNAL_LIB_PATH}/json/include)
include_directories(${EXTERNAL_LIB_PATH}/websocketpp)

list(APPEND ALL_CPP_FILES
    "src/webdash-config.cpp"
    "src/webdash-config-task.cpp"
    "src/webdash-core.cpp"
)

ADD_LIBRARY(webdash-executer STATIC ${ALL_CPP_FILES} )
target_link_libraries(webdash-executer Boost::filesystem)

