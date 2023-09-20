cmake_minimum_required(VERSION 3.26)
project(wat24)


set(SRC
./src/main.cpp
"src/renderer.cpp"
"src/primitive.cpp"
./src/DDS.cpp
 "src/water.h" "src/water.cpp")

add_executable(
  test
  WIN32
  ${SRC}
)

target_link_libraries(test PRIVATE
    d3d11.lib dxgi.lib dxguid.lib uuid.lib
    d3dcompiler.lib
    kernel32.lib user32.lib
    comdlg32.lib advapi32.lib shell32.lib
    ole32.lib oleaut32.lib gdi32.lib
    winmm.lib
)

