project(wheel)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "C:/mingw32/include/boost-1_54")
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "C:/mingw32/lib")
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost COMPONENTS system filesystem thread chrono REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

add_definitions(-DDEBUG)
set(CMAKE_CXX_FLAGS "-g -O0 -Wall -Wextra -Werror -std=c++11 ${CMAKE_CXX_FLAGS}")

include_directories(SYSTEM $ENV{LOCAL_DIR}/include/freetype2)

FILE(COPY cube.ply DESTINATION .)
FILE(COPY LiberationSans-Regular.ttf DESTINATION .)

set(SRC_LIST
Tetris.h
Tetris.cpp
Program.h
Program.cpp
Window.h
Window.cpp
Random.h
Text.h
Text.cpp
Config.h
Config.cpp
entry.h
entry.cpp
rstd.h
vformat.h
)

set(WINDOWS_LIB_DEPS
    glew32
    opengl32
)

set(LINUX_LIB_DEPS
    GLEW
    GL
    X11
    Xxf86vm
    Xrandr
    Xi
    pthread
    bz2
)

set(LIBS ${LINUX_LIB_DEPS})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(LIBS ${WINDOWS_LIB_DEPS})
endif()

add_library(${PROJECT_NAME} ${SRC_LIST})

project(desktop)
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME}
    wheel
    glfw3
    assimp
    freetype
    freeimage
    ${LIBS}    
    ${Boost_LIBRARIES}
)

add_executable(tests tests.cpp)
target_link_libraries(tests wheel gtest pthread ${Boost_LIBRARIES})
