cmake_minimum_required(VERSION 3.20)
project(SoftRenderer LANGUAGES C CXX)

# The desktop and UWP/Xbox targets use the same statically linked renderer
# sources. UWP has a dedicated packaged host because it has no MiniFB window.
if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    add_subdirectory(uwp)
    return()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
# The desktop executable owns the complete renderer and its windowing
# dependency. Do not let a parent build turn MiniFB into another DLL.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SoftRenderer dependencies statically" FORCE)
set(MINIFB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIFB_BUILD_VERSION_INFO OFF CACHE BOOL "" FORCE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(SR_LINUX_WAYLAND "Use MiniFB's native Wayland backend on Linux" ON)
    set(MINIFB_USE_WAYLAND_API ${SR_LINUX_WAYLAND} CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(minifb
    GIT_REPOSITORY https://github.com/emoon/minifb.git
    GIT_TAG v0.10.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(minifb)

find_package(Threads REQUIRED)

set(SR_CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/code")
set(SR_CORE_DIR "${SR_CODE_DIR}/SoftRenderer")
set(SR_RENDERER_DIR "${SR_CODE_DIR}/SrSwRendererCpu")
set(SR_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(SR_COMMON_INCLUDE_DIRS "${SR_CORE_DIR}" "${SR_RENDERER_DIR}")

function(sr_configure_target target)
    target_include_directories(${target} PRIVATE
        ${SR_COMMON_INCLUDE_DIRS}
        "${SR_CODE_DIR}/BasicShaders"
        "${SR_CODE_DIR}/AdvanceShaders"
    )
    if(MSVC)
        target_compile_options(${target} PRIVATE /W3 /fp:fast)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
		# Anonymous unions preserve the renderer's POD/SIMD layout; the
		# corresponding aggregate-brace and legacy member-order diagnostics are
		# intentional for these low-level compatibility types.
		target_compile_options(${target} PRIVATE -Wall -Wno-unused-variable -Wno-unused-function -Wno-missing-braces -ffast-math -fms-extensions)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(${target} PRIVATE -Wno-reorder-ctor)
		elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			target_compile_options(${target} PRIVATE -Wno-reorder)
		endif()
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            target_compile_definitions(${target} PRIVATE SR_USE_SIMD)
        endif()
    endif()
endfunction()

function(sr_set_runtime_layout target subdirectory)
    set(output_directory "${SR_RUNTIME_DIR}")
    if(NOT "${subdirectory}" STREQUAL "")
        set(output_directory "${output_directory}/${subdirectory}")
    endif()
    set_target_properties(${target} PROPERTIES
        # The always-true generator expression prevents Visual Studio and
        # other multi-config generators from appending a Release subfolder.
        RUNTIME_OUTPUT_DIRECTORY "$<1:${output_directory}>"
        LIBRARY_OUTPUT_DIRECTORY "$<1:${output_directory}>"
    )
endfunction()

set(SR_APP_SOURCES
    "${SR_CORE_DIR}/InputManager.cpp"
    "${SR_CORE_DIR}/mmgr/mmgr.cpp"
    "${SR_CORE_DIR}/SoftRenderApp.cpp"
    "${SR_CORE_DIR}/main.cpp"
    "${SR_CORE_DIR}/SrBitmap.cpp"
    "${SR_CORE_DIR}/SrCamera.cpp"
    "${SR_CORE_DIR}/SrDefaultMedia.cpp"
    "${SR_CORE_DIR}/SrEntity.cpp"
    "${SR_CORE_DIR}/SrLogger.cpp"
    "${SR_CORE_DIR}/SrMaterial.cpp"
    "${SR_CORE_DIR}/SrMesh.cpp"
    "${SR_CORE_DIR}/SrModelViewerApp.cpp"
    "${SR_CORE_DIR}/SrObjLoader.cpp"
    "${SR_CORE_DIR}/SrProfiler.cpp"
    "${SR_CORE_DIR}/SrRenderTexture.cpp"
    "${SR_CORE_DIR}/SrResource.cpp"
    "${SR_CORE_DIR}/SrResourceManager.cpp"
    "${SR_CORE_DIR}/SrScene.cpp"
    "${SR_CORE_DIR}/SrSponzaApp.cpp"
    "${SR_CORE_DIR}/SrTexture.cpp"
    "${SR_CORE_DIR}/stdafx.cpp"
    "${SR_CORE_DIR}/util/memfile.cpp"
)

set(SR_RENDERER_SOURCES
    "${SR_RENDERER_DIR}/SrFragmentBuffer.cpp"
    "${SR_RENDERER_DIR}/SrRasTaskDispatcher.cpp"
    "${SR_RENDERER_DIR}/SrRasTasks.cpp"
    "${SR_RENDERER_DIR}/SrRasterizer.cpp"
    "${SR_RENDERER_DIR}/SrRasterizer_Algo.cpp"
    "${SR_RENDERER_DIR}/SrRasterize_Clip.cpp"
    "${SR_RENDERER_DIR}/SrSoftRenderer.cpp"
    "${SR_RENDERER_DIR}/SrTileRenderer.cpp"
    "${SR_RENDERER_DIR}/SrTileBinner.cpp"
)

set(SR_SHADER_SOURCES
    "${SR_CODE_DIR}/BasicShaders/BasicShaders.cpp"
    "${SR_CODE_DIR}/AdvanceShaders/AdvanceShaders.cpp"
)

# The renderer and all software shaders are part of the main executable. No
# renderer/ or shader/ runtime plugin directories are produced or scanned.
add_executable(SoftRenderer
    ${SR_APP_SOURCES}
    ${SR_RENDERER_SOURCES}
    ${SR_SHADER_SOURCES}
)
sr_configure_target(SoftRenderer)
target_compile_definitions(SoftRenderer PRIVATE SR_STATIC_BUILD)
target_link_libraries(SoftRenderer PRIVATE minifb Threads::Threads)
sr_set_runtime_layout(SoftRenderer "")

enable_testing()
add_executable(SoftRendererTests
    "${SR_RENDERER_DIR}/tests/SrTileTests.cpp"
    "${SR_RENDERER_DIR}/SrTileBinner.cpp"
    "${SR_CORE_DIR}/SrTexture.cpp"
)
target_include_directories(SoftRendererTests PRIVATE "${SR_RENDERER_DIR}" "${SR_CORE_DIR}")
sr_set_runtime_layout(SoftRendererTests "")
add_test(NAME SoftRenderer.TileLayout COMMAND SoftRendererTests)

install(TARGETS SoftRenderer RUNTIME DESTINATION .)
