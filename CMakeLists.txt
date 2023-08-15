cmake_minimum_required(VERSION 3.1)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

project(wat24)

find_package(directxtk CONFIG REQUIRED)

set(SRC
./src/main.cpp
./src/renderer.cpp
)

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
    ole32.lib oleaut32.lib
    Microsoft::DirectXTK
)

