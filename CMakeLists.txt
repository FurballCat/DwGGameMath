cmake_minimum_required(VERSION 3.10)
project(game-math)

# use C++11 standard
set (CMAKE_CXX_STANDARD 11)

# suppress generating ZERO_CHECK project
set (CMAKE_SUPPRESS_REGENERATION true)

# add source files
file(GLOB_RECURSE SOURCE_FILES 
	${CMAKE_SOURCE_DIR}/src/*.c
	${CMAKE_SOURCE_DIR}/src/*.cpp)
	
# add header files
file(GLOB_RECURSE HEADER_FILES 
	${CMAKE_SOURCE_DIR}/src/*.h
	${CMAKE_SOURCE_DIR}/src/*.hpp)
	
include_directories(${CMAKE_SOURCE_DIR}/lib/glfw/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/glad/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/vectormath)

add_executable(${PROJECT_NAME} WIN32 ${HEADER_FILES} ${SOURCE_FILES})
add_library(glad "lib/glad/src/glad.c")

link_directories(${CMAKE_SOURCE_DIR}/lib)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/lib/glfw/lib/glfw3.lib glad)
