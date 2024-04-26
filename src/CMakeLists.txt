set(
  SRC
  main.cpp
  utils.cpp
  ../include/utils.h
  renderer.cpp
  DDS.cpp
  cubemap.cpp
  surface.cpp
  device-resources.cpp
  simulation/marching-cubes.cpp
  simulation/water.cpp
  simulationRenderer.cpp
  simulation/heightfield.cpp
  simulation/sph.cpp
  simulation/sph-gpu.cpp
  simulation/settings.h
)

add_executable(${PROJECT_NAME} WIN32
    ${SRC}
    # ${CPP_HEADER_FILES}
    ${CPP_SOURCE_FILES}
)

# headers and paths
set(CPP_INCLUDE_DIRS "")
set(CPP_SOURCE_FILES "")
set(CPP_HEADER_FILES "")

include(FetchContent)
set(FETCHCONTENT_QUIET NO)

FetchContent_Declare(
  directxtk
  GIT_REPOSITORY https://github.com/microsoft/DirectXTK
  GIT_TAG main
)
FetchContent_MakeAvailable(directxtk)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.82
)
FetchContent_MakeAvailable(imgui)

list(APPEND CPP_INCLUDE_DIRS ${imgui_SOURCE_DIR})
list(APPEND CPP_INCLUDE_DIRS ${imgui_SOURCE_DIR}/backends)

list(APPEND CPP_SOURCE_FILES ${imgui_SOURCE_DIR}/imgui.cpp)
list(APPEND CPP_SOURCE_FILES ${imgui_SOURCE_DIR}/imgui_demo.cpp)
list(APPEND CPP_SOURCE_FILES ${imgui_SOURCE_DIR}/imgui_draw.cpp)
list(APPEND CPP_SOURCE_FILES ${imgui_SOURCE_DIR}/imgui_tables.cpp)
list(APPEND CPP_SOURCE_FILES ${imgui_SOURCE_DIR}/imgui_widgets.cpp)

list(APPEND CPP_SOURCE_FILES ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp)
list(APPEND CPP_SOURCE_FILES ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
#list(APPEND CPP_SOURCE_FILES ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp)

list(APPEND CPP_HEADER_FILES ${imgui_SOURCE_DIR}/imgui.h)
list(APPEND CPP_HEADER_FILES ${imgui_SOURCE_DIR}/imconfig.h)
list(APPEND CPP_HEADER_FILES ${imgui_SOURCE_DIR}/imgui_internal.h)

list(APPEND CPP_HEADER_FILES ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.h)
list(APPEND CPP_HEADER_FILES ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.h)
#list(APPEND CPP_HEADER_FILES ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.h)

target_sources(${PROJECT_NAME} PUBLIC ${CPP_SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib dxgi.lib dxguid.lib
    d3dcompiler.lib DirectXTK
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/simulation
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/simulation
    ${directxtk_INCLUDE_DIR}
    ${CPP_INCLUDE_DIRS}
)
