cmake_minimum_required (VERSION 3.15)
project(wheel)

set(wheel_VERSION "0.2.0")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Werror>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-march=x86-64-v3>)

#set(CMAKE_CXX_FLAGS "-fsanitize=address -fsanitize=undefined ${CMAKE_CXX_FLAGS}")

find_package(boost_system REQUIRED CONFIG)
find_package(Freetype REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glm REQUIRED)
find_package(pugixml REQUIRED)

include_directories(${PROJECT_SOURCE_DIR})

file(COPY config.xml DESTINATION .)
file(COPY cube.ply DESTINATION .)
file(COPY LiberationSans-Regular.ttf DESTINATION .)
file(COPY lang.en.xml DESTINATION .)

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
    Program.cpp
    Window.cpp
    Text.cpp
    Config.cpp
    Keyboard.cpp
    simulator.cpp
)

add_library(wheel-lib STATIC ${SRC_LIST})
target_link_libraries(wheel-lib
    assimp::assimp
    Freetype::Freetype
    GLEW::GLEW
    SDL2::SDL2
    SDL2::SDL2main
    OpenGL::GL
    Boost::system
    glm::glm
    pugixml::pugixml
)

add_executable(wheel WIN32 entry.cpp)
target_link_libraries(wheel wheel-lib)

if(NOT WIN32)
    add_executable(tests tests.cpp)
    target_link_libraries(tests wheel-lib gtest pthread)
endif()

install(FILES
    config.xml
    lang.en.xml
    lang.ru.xml
    cube.ply
    LiberationSans-Regular.ttf
    DESTINATION /)
install(TARGETS wheel DESTINATION /)
install(FILES $<TARGET_RUNTIME_DLLS:wheel> DESTINATION /)

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
else()
    install(TARGETS wheel DESTINATION bin)
endif()
