== build glfw >3.3

mingw32-cmake -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES="/usr/i686-w64-mingw32/sys-root/mingw/include" \
    .
ninja
sudo ninja install

== build wheel

mingw32-cmake -GNinja \
    -DCMAKE_RELEASE=TRUE \
    -DCMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES="/usr/i686-w64-mingw32/sys-root/mingw/include" \
    .
ninja package
