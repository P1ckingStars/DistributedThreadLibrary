cmake_minimum_required(VERSION 3.10)

enable_language(ASM)
# set the project name and version
project(DSMProject VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# configure a header file to pass some of the CMake settings
# to the source code

add_subdirectory(src/lib)

add_compile_options(-fno-stack-protector)
# add the executable
add_executable(test2 examples/test2.cpp)

target_compile_options(test2 PUBLIC -fno-stack-protector)

target_link_options(test2 PRIVATE
    "-T${CMAKE_SOURCE_DIR}/linker.ld"
)


link_directories(${PROJECT_SOURCE_DIR}/dependency)

target_link_libraries(test2 DSMLib rpc)

target_include_directories(test2 PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/src/include"
                          )

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
