cmake_minimum_required(VERSION 2.8.9)
project (test)

message ("$ENV{OPENCV4_ROOT} ")
message ("$ENV{TENSORRT_ROOT}")
message ("$ENV{CUDA_LIB_PATH}")
message ("$ENV{CUDA_PATH}")


set (CMAKE_BUILD_TYPE release)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS}  -O3 -Wall -DNDEBUG")

include_directories($ENV{OPENCV4_ROOT}/include include $ENV{TENSORRT_ROOT}/include $ENV{CUDA_PATH}/include)

set (LIB_SRC src/qndeblock-sdk.cpp src/qndeblock.cpp )
file(GLOB TEST_SRC test/*.c test/*.cpp)

#Generate the static library from the sources
add_library(qndeblock STATIC ${LIB_SRC})

set ( PROJECT_LINK_LIBS libqndeblock.a libstdc++.a libopencv_world411.dll.a nvonnxparser.lib nvinfer.lib cuda.lib cudnn.lib cudart.lib )
link_directories( $ENV{OPENCV4_ROOT}/lib $ENV{TENSORRT_ROOT}/lib $ENV{CUDA_LIB_PATH} ./build )

add_executable(test ${TEST_SRC})
target_link_libraries(test ${PROJECT_LINK_LIBS} )

SET(CMAKE_INSTALL_PREFIX ../)
INSTALL ( TARGETS qndeblock 
			ARCHIVE DESTINATION lib )


