project(curv)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267") # int64 -> int
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244") # float -> int
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4291") # no matching operator delete found


FILE(GLOB CurvSrc "curv/*.cc")
FILE(GLOB CurvInc "curv/*.h")
add_library(curv ${CurvSrc} ${CurvInc})

target_link_libraries(curv PUBLIC boost double-conversion)
target_include_directories(curv PUBLIC .)

target_compile_features(curv PUBLIC cxx_std_17)
target_compile_definitions(curv PUBLIC "_HAS_AUTO_PTR_ETC")
