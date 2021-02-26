cmake_minimum_required(VERSION 2.8.9)
project (test)

message ("$ENV{OPENCV4_ROOT} ")

include_directories($ENV{OPENCV4_ROOT}/include include)

set (LIB_SRC src/qnfilter-sdk.cpp)
file(GLOB TEST_SRC test/*.c test/*.cpp)

#Generate the static library from the sources
add_library(qnfilter STATIC ${LIB_SRC})

set ( PROJECT_LINK_LIBS libqnfilter.a libstdc++.a libopencv_world411.dll.a  )
link_directories( $ENV{OPENCV4_ROOT}/lib ./build)

add_executable(test ${TEST_SRC})
target_link_libraries(test ${PROJECT_LINK_LIBS} )

SET(CMAKE_INSTALL_PREFIX ../)
INSTALL ( TARGETS qnfilter 
			ARCHIVE DESTINATION lib )


