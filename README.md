dnf install mingw32-gcc-c++ mingw32-boost mingw32-SDL2 mingw32-minizip

== build assimp 4.1.0

mingw32-cmake -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES="/usr/i686-w64-mingw32/sys-root/mingw/include" \
    -DASSIMP_BUILD_ASSIMP_TOOLS=FALSE \
    .
ninja install

== build wheel

mingw32-cmake -GNinja \
    -DCMAKE_RELEASE=TRUE \
    -DCMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES="/usr/i686-w64-mingw32/sys-root/mingw/include" \
    .
ninja package
