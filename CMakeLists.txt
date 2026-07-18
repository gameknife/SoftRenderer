cmake_minimum_required(VERSION 3.20)
project(SoftRenderer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
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
    target_include_directories(${target} PRIVATE ${SR_COMMON_INCLUDE_DIRS})
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
        ARCHIVE_OUTPUT_DIRECTORY "$<1:${output_directory}>"
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

add_executable(SoftRenderer ${SR_APP_SOURCES})
sr_configure_target(SoftRenderer)
target_link_libraries(SoftRenderer PRIVATE minifb Threads::Threads ${CMAKE_DL_LIBS})
if(APPLE)
    target_link_options(SoftRenderer PRIVATE -Wl,-export_dynamic)
endif()
sr_set_runtime_layout(SoftRenderer "")

set(SR_RENDERER_SOURCES
    "${SR_RENDERER_DIR}/module_def.cpp"
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
add_library(SrSwRendererCpu SHARED ${SR_RENDERER_SOURCES})
sr_configure_target(SrSwRendererCpu)
target_link_libraries(SrSwRendererCpu PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
if(WIN32)
    target_link_libraries(SrSwRendererCpu PRIVATE gdi32)
endif()
set_target_properties(SrSwRendererCpu PROPERTIES PREFIX "")
sr_set_runtime_layout(SrSwRendererCpu "renderer")

add_library(BasicShaders SHARED "${SR_CODE_DIR}/BasicShaders/BasicShaders.cpp")
sr_configure_target(BasicShaders)
set_target_properties(BasicShaders PROPERTIES PREFIX "" OUTPUT_NAME BasicShaders SUFFIX ".swsl")
sr_set_runtime_layout(BasicShaders "shader")

add_library(AdvanceShaders SHARED "${SR_CODE_DIR}/AdvanceShaders/AdvanceShaders.cpp")
sr_configure_target(AdvanceShaders)
set_target_properties(AdvanceShaders PROPERTIES PREFIX "" OUTPUT_NAME AdvanceShaders SUFFIX ".swsl")
sr_set_runtime_layout(AdvanceShaders "shader")

if(APPLE)
    foreach(target SrSwRendererCpu BasicShaders AdvanceShaders)
        target_link_options(${target} PRIVATE -Wl,-undefined,dynamic_lookup)
    endforeach()
endif()

add_dependencies(SoftRenderer SrSwRendererCpu BasicShaders AdvanceShaders)

enable_testing()
add_executable(SoftRendererTests "${SR_RENDERER_DIR}/tests/SrTileTests.cpp" "${SR_RENDERER_DIR}/SrTileBinner.cpp")
target_include_directories(SoftRendererTests PRIVATE "${SR_RENDERER_DIR}")
sr_set_runtime_layout(SoftRendererTests "")
add_test(NAME SoftRenderer.TileLayout COMMAND SoftRendererTests)

install(TARGETS SoftRenderer RUNTIME DESTINATION .)
