cmake_minimum_required(VERSION 3.1)
project(wat24)

set(SRC
./src/main.cpp
./src/renderer.cpp
)


add_executable(
  test
  WIN32
  ${SRC}
)

target_link_libraries(test gdi32 user32 winmm d3d11 d3dcompiler dxgi dxguid)

