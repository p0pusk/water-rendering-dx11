cmake_minimum_required(VERSION 3.26)
project(wat24)

# set(CMAKE_CXX_STANDART 20)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/Library/DirectXTK ${CMAKE_BINARY_DIR}/bin/CMake/DirectXTK)


set(SRC
"src/main.cpp"
"src/renderer.cpp"
"src/surface.cpp"
./src/DDS.cpp
./src/simulation/neighbour-hash.cpp
./src/simulation/marching-cubes.cpp
"src/simulation/water.cpp"
 "src/simulation/sph.cpp"
 "src/dx-controller.cpp"
 "src/cubemap.cpp"
 "src/primitive.cpp"
)


add_executable(
  water
  WIN32
  ${SRC}
 "src/heightfield-test.cpp"
 "src/simulation/heightfield.cpp")

 add_executable(
  test
	 "src/sph-test.cpp"
	 "src/simulation/sph.cpp"
 )

 target_link_libraries(
     test
     DirectXTK
 )

target_include_directories(water PRIVATE "./inc")
target_include_directories(test PRIVATE "./inc")

target_link_libraries(water PRIVATE
    d3d11.lib dxgi.lib dxguid.lib uuid.lib
    d3dcompiler.lib
    kernel32.lib user32.lib
    comdlg32.lib advapi32.lib shell32.lib
    ole32.lib oleaut32.lib gdi32.lib
    winmm.lib DirectXTK
)

