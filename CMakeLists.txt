cmake_minimum_required(VERSION 3.11)
project(wheel)

set(wheel_VERSION "0.1.0")

set(CMAKE_CXX_STANDARD 17)

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
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror ${CMAKE_CXX_FLAGS}")

find_package(PkgConfig REQUIRED)
pkg_search_module(FREETYPE2 REQUIRED freetype2)

if(WIN32)
    include_directories(SYSTEM /usr/i686-w64-mingw32/sys-root/mingw/include/freetype2)
else()
    include_directories(SYSTEM ${FREETYPE2_INCLUDE_DIRS})
endif()

file(COPY config.xml DESTINATION .)
file(COPY cube.ply DESTINATION .)
file(COPY LiberationSans-Regular.ttf DESTINATION .)
file(COPY lang.en.xml DESTINATION .)
file(COPY lang.ru.xml DESTINATION .)

add_definitions(-DGLM_FORCE_CTOR_INIT)

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
    HighscoreManager.h
    HighscoreManager.cpp
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
    Bitmap.h
    Bitmap.cpp
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
    rstd.h
    vformat.h
)

if(WIN32)
    set(LIB_DEPS
        glfw3
        glew32
        opengl32
        z
    )
else()
    set(LIB_DEPS
        GLEW
        GL
        X11
        Xxf86vm
        Xrandr
        Xi
        pthread
        bz2
        glfw
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
