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
FILE(COPY lang.en.xml DESTINATION .)
FILE(COPY lang.ru.xml DESTINATION .)

set(SRC_LIST
Camera.h
Camera.cpp
MathTools.h
MathTools.cpp
Trunk.h
Trunk.cpp
Mesh.h
Mesh.cpp
ScaleAnimation.h
ScaleAnimation.cpp
OpenGLbasics.h
OpenGLbasics.cpp
BindLock.h
Texture.h
Texture.cpp
Widgets/CrispBitmap.h
Widgets/CrispBitmap.cpp
Widgets/IWidget.h
Widgets/SpreadAnimator.h
Widgets/SpreadAnimator.cpp
Widgets/TextLine.h
Widgets/TextLine.cpp
Widgets/HudList.h
Widgets/HudList.cpp
Widgets/MenuLeaf.h
Widgets/MenuLeaf.cpp
Widgets/Menu.h
Widgets/Menu.cpp
Widgets/HighscoreScreen.h
Widgets/HighscoreScreen.cpp
Widgets/MenuController.h
Widgets/MenuController.cpp
Widgets/Painter2D.h
Widgets/Painter2D.cpp
Widgets/TextEdit.h
Widgets/TextEdit.cpp
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
Keyboard.h
Keyboard.cpp
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
