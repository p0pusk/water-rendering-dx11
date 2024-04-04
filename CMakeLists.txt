cmake_minimum_required(VERSION 3.20)
project(wat24)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC
./src/main.cpp
./src/renderer.cpp
./src/DDS.cpp
./src/simulation/marching-cubes.cpp
# ./src/simulation/marching-cubes-gpu.cpp
./src/simulation/water.cpp
./src/simulationRenderer.cpp
./src/simulation/heightfield.cpp
./src/cubemap.cpp
./src/simulation/settings.h
./src/voxelGridRenderer.h
./src/surface.cpp
 ./src/simulation/sph.cpp
 ./src/device-resources.cpp
 ./src/primitive.cpp)


option(BUILD_TEST_TEMPLATE "Ignore warnings related to TODOs" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

#testing...
set(ENABLE_DIRECTXTK ON)
set(ENABLE_IMGUI ON)

# headers and paths
set(CPP_INCLUDE_DIRS "")
set(CPP_SOURCE_FILES "")
set(CPP_HEADER_FILES "")


if(ENABLE_DIRECTXTK)
  if (NOT directxtk_FOUND) # If there's none, fetch and build
    include(FetchContent)
    FetchContent_Declare(
      directxtk
      GIT_REPOSITORY https://github.com/microsoft/DirectXTK
      GIT_TAG main
    )
    FetchContent_GetProperties(directxtk)
    if (NOT directxtk_POPULATED) # Have we downloaded raylib yet?
      set(FETCHCONTENT_QUIET NO)
      FetchContent_Populate(directxtk)
      # message("directxtk_LIBRARY: " ${directxtk_LIBRARY}) # fail but define in cmake
      # message("directxtk_INCLUDE_DIR: " ${directxtk_INCLUDE_DIR}) #pass
      # message("directxtk_SOURCE_DIR: " ${directxtk_SOURCE_DIR}) # pass
      # message("directxtk_BINARY_DIR: " ${directxtk_BINARY_DIR}) # pass
      # set(FT_WITH_ZLIB OFF CACHE BOOL "" FORCE)
      add_subdirectory(${directxtk_SOURCE_DIR} ${directxtk_BINARY_DIR}) #add to build in sub dir
      list(APPEND CPP_INCLUDE_DIRS ${directxtk_SOURCE_DIR}/Inc)
    endif()
  endif()
endif()

if(ENABLE_IMGUI)
  if (NOT imgui_FOUND) # If there's none, fetch and build imgui
    include(FetchContent)
    FetchContent_Declare(
      imgui
      GIT_REPOSITORY https://github.com/ocornut/imgui.git
      GIT_TAG v1.82
    )
    FetchContent_GetProperties(imgui)
    if (NOT imgui_POPULATED) # Have we downloaded yet?
      set(FETCHCONTENT_QUIET NO)
      FetchContent_Populate(imgui)
      # utility to setup the downloaded library for use
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

    endif()
  endif()
endif()

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)

if(DEFINED VCPKG_TARGET_ARCHITECTURE)
    set(DIRECTX_ARCH ${VCPKG_TARGET_ARCHITECTURE})
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
endif()

add_executable(${PROJECT_NAME} WIN32
    ${SRC}
    ${CPP_HEADER_FILES}
    ${CPP_SOURCE_FILES}
)

if(NOT MINGW)
    # target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib dxgi.lib dxguid.lib
    d3dcompiler.lib DirectXTK
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ./src
    ./src/simulation
    ${CPP_INCLUDE_DIRS}
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Wall /GR "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
    target_link_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>" /DYNAMICBASE /NXCOMPAT)

    if((CMAKE_SIZEOF_VOID_P EQUAL 4) AND (NOT ${DIRECTX_ARCH} MATCHES "^arm"))
        target_link_options(${PROJECT_NAME} PRIVATE /SAFESEH)
    endif()

    if((MSVC_VERSION GREATER_EQUAL 1924)
       AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)))
      target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
    endif()

    if((MSVC_VERSION GREATER_EQUAL 1928)
       AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
       AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)))
        target_compile_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
        target_link_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
    endif()
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG>)
endif()

if(NOT ${DIRECTX_ARCH} MATCHES "^arm")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    endif()

    target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_SSE2})
endif()

if(MINGW)
    find_package(directxmath CONFIG REQUIRED)
else()
    find_package(directxmath CONFIG QUIET)
endif()


if( CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM" )
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wpedantic -Wextra
        "-Wno-exit-time-destructors" "-Wno-global-constructors" "-Wno-language-extension-token"
        "-Wno-missing-prototypes" "-Wno-missing-variable-declarations" "-Wno-reserved-id-macro")

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-unsafe-buffer-usage")
    endif()

    if(BUILD_TEST_TEMPLATE)
        target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-unused-value")
    endif()
elseif(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-ignored-attributes)
    target_link_options(${PROJECT_NAME} PRIVATE -municode)
elseif( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options(${PROJECT_NAME} PRIVATE
         /sdl /permissive- /Zc:__cplusplus /Zc:inline /fp:fast
         "/wd4061" "/wd4365" "/wd4514" "/wd4571" "/wd4668" "/wd4710" "/wd4820"
         "/wd5039" "/wd5045")

    if(ENABLE_CODE_ANALYSIS)
        target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
    endif()

    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        target_compile_options(${PROJECT_NAME} PRIVATE /Gy /Gw)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /wd5105)
    endif()

    if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
        target_link_options(${PROJECT_NAME} PRIVATE /CETCOMPAT)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:lambda)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
        target_compile_options(${PROJECT_NAME} PRIVATE /wd5262 /wd5264)
    endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:checkGwOdr $<$<VERSION_GREATER_EQUAL:${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION},10.0.22000>:/Zc:templateScope>)
    endif()

    if(BUILD_TEST_TEMPLATE)
        target_compile_options(${PROJECT_NAME} PRIVATE "/wd4555")
    endif()
endif()
if( CMAKE_CXX_COMPILER_ID MATCHES "^Intel$" )
    target_compile_options(${PROJECT_NAME} PRIVATE /Qwd161)
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE)

    if(${DIRECTX_ARCH} MATCHES "^arm")
        target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0601)
    endif()
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
