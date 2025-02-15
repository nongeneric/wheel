cmake_minimum_required(VERSION 3.11)
project(wheel)

set(wheel_VERSION "0.1.2")

set(CMAKE_CXX_STANDARD 23)

set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost COMPONENTS system filesystem thread chrono REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

if(DEFINED CMAKE_RELEASE)
    set(CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")
    if(WIN32)
        set(CMAKE_CXX_FLAGS "-Wl,--subsystem,windows ${CMAKE_CXX_FLAGS}")
    endif()
else()
    add_definitions(-DDEBUG)
    set(CMAKE_CXX_FLAGS "-ggdb -O0 ${CMAKE_CXX_FLAGS}")
endif()
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -march=x86-64-v3 ${CMAKE_CXX_FLAGS}")

find_package(PkgConfig REQUIRED)
pkg_search_module(FREETYPE2 REQUIRED freetype2)

if(WIN32)
    include_directories(SYSTEM /usr/i686-w64-mingw32/sys-root/mingw/include/freetype2)
else()
    include_directories(SYSTEM ${FREETYPE2_INCLUDE_DIRS})
endif()

include_directories(${PROJECT_SOURCE_DIR})

file(COPY config.xml DESTINATION .)
file(COPY cube.ply DESTINATION .)
file(COPY LiberationSans-Regular.ttf DESTINATION .)
file(COPY lang.en.xml DESTINATION .)
file(COPY lang.ru.xml DESTINATION .)

add_definitions(-DGLM_FORCE_CTOR_INIT)
add_definitions(-DGLM_FORCE_CXX11)

set(SRC_LIST
    Camera.cpp
    MathTools.cpp
    Trunk.cpp
    Mesh.cpp
    ScaleAnimation.cpp
    OpenGLbasics.cpp
    Texture.cpp
    HighscoreManager.cpp
    Widgets/CrispBitmap.cpp
    Widgets/SpreadAnimator.cpp
    Widgets/TextLine.cpp
    Widgets/HudList.cpp
    Widgets/MenuLeaf.cpp
    Widgets/Menu.cpp
    Widgets/HighscoreScreen.cpp
    Widgets/MenuController.cpp
    Widgets/Painter2D.cpp
    Widgets/TextEdit.cpp
    Bitmap.cpp
    AiTetris.cpp
    Tetris.cpp
    Time.cpp
    Program.cpp
    Window.cpp
    Text.cpp
    Config.cpp
    Keyboard.cpp
    simulator.cpp
)

if(WIN32)
    set(LIB_DEPS
        glew32
        mingw32
        SDL2main
        SDL2
        opengl32
        z
    )
else()
    set(LIB_DEPS
        GLEW
        GL
        SDL2
        X11
        Xxf86vm
        Xrandr
        Xi
        pthread
        bz2
    )
endif()

add_library(wheel-lib STATIC ${SRC_LIST})
target_link_libraries(wheel-lib
    assimp
    freetype
    ${LIB_DEPS}
    ${Boost_LIBRARIES}
)

add_executable(wheel entry.cpp)
target_link_libraries(wheel wheel-lib)

if(NOT WIN32)
    add_executable(tests tests.cpp)
    target_link_libraries(tests wheel-lib gtest pthread ${Boost_LIBRARIES})
endif()

if(WIN32)
    configure_file (
        "${CMAKE_SOURCE_DIR}/install.cmake.in"
        "${CMAKE_BINARY_DIR}/install.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_BINARY_DIR}/install.cmake")
    set(CPACK_PACKAGE_FILE_NAME "wheel-${wheel_VERSION}-win32")
    set(CPACK_GENERATOR "ZIP")
    include(CPack)
endif()
