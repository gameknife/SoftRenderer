cmake_minimum_required(VERSION 3.20)

project(SoftRenderer LANGUAGES CXX RC)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "SoftRenderer requires the Microsoft Visual C++ toolchain on Windows.")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SR_CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/code")
set(SR_CORE_DIR "${SR_CODE_DIR}/SoftRenderer")
set(SR_RENDERER_DIR "${SR_CODE_DIR}/SrSwRendererCpu")
set(SR_THIRDPARTY_DIR "${SR_CODE_DIR}/thirdparty")
set(SR_MEDIA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin32/media")
set(SR_RUNTIME_DIR "${CMAKE_BINARY_DIR}/runtime")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SR_DXSDK_ARCH x64)
else()
    set(SR_DXSDK_ARCH x86)
endif()

find_library(SR_DXGUID_LIBRARY
    NAMES dxguid
    PATHS "${SR_THIRDPARTY_DIR}/dxsdk/lib/${SR_DXSDK_ARCH}"
    NO_DEFAULT_PATH
    REQUIRED
)

set(SR_COMMON_INCLUDE_DIRS
    "${SR_CORE_DIR}"
    "${SR_RENDERER_DIR}"
    "${SR_THIRDPARTY_DIR}/dxsdk/include"
)

function(sr_configure_target target)
    target_include_directories(${target} PRIVATE ${SR_COMMON_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE
        WIN32
        _WINDOWS
        _MBCS
        _CRT_SECURE_NO_WARNINGS
        SR_USE_SIMD
    )

    target_compile_options(${target} PRIVATE
        /W3
        /fp:fast
    )
endfunction()

function(sr_set_runtime_layout target subdirectory)
    set(output_directory "${SR_RUNTIME_DIR}/$<CONFIG>")
    if(NOT "${subdirectory}" STREQUAL "")
        set(output_directory "${output_directory}/${subdirectory}")
    endif()

    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${output_directory}"
        LIBRARY_OUTPUT_DIRECTORY "${output_directory}"
        ARCHIVE_OUTPUT_DIRECTORY "${output_directory}"
    )
endfunction()

set(SR_APP_SOURCES
    "${SR_CORE_DIR}/InputManager.cpp"
    "${SR_CORE_DIR}/Keyboard.cpp"
    "${SR_CORE_DIR}/mmgr/mmgr.cpp"
    "${SR_CORE_DIR}/Mouse.cpp"
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
    "${SR_CORE_DIR}/SoftRenderer.rc"
)

add_executable(SoftRenderer WIN32 ${SR_APP_SOURCES})
sr_configure_target(SoftRenderer)
target_link_libraries(SoftRenderer PRIVATE
    d3d9
    dinput8
    winmm
    opengl32
    "${SR_DXGUID_LIBRARY}"
)
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
target_compile_definitions(SrSwRendererCpu PRIVATE SRSWRENDERERCPU_EXPORTS)
target_link_libraries(SrSwRendererCpu PRIVATE
    d3d9
    "${SR_DXGUID_LIBRARY}"
)
sr_set_runtime_layout(SrSwRendererCpu "renderer")

add_library(BasicShaders SHARED
    "${SR_CODE_DIR}/BasicShaders/BasicShaders.cpp"
)
sr_configure_target(BasicShaders)
target_compile_definitions(BasicShaders PRIVATE BASICSHADERS_EXPORTS)
set_target_properties(BasicShaders PROPERTIES OUTPUT_NAME BasicShaders SUFFIX ".swsl")
sr_set_runtime_layout(BasicShaders "shader")

add_library(AdvanceShaders SHARED
    "${SR_CODE_DIR}/AdvanceShaders/AdvanceShaders.cpp"
)
sr_configure_target(AdvanceShaders)
target_compile_definitions(AdvanceShaders PRIVATE ADVANCESHADERS_EXPORTS)
set_target_properties(AdvanceShaders PROPERTIES OUTPUT_NAME AdvanceShaders SUFFIX ".swsl")
sr_set_runtime_layout(AdvanceShaders "shader")

add_dependencies(SoftRenderer SrSwRendererCpu BasicShaders AdvanceShaders)

enable_testing()
add_executable(SoftRendererTests
	"${SR_RENDERER_DIR}/tests/SrTileTests.cpp"
	"${SR_RENDERER_DIR}/SrTileBinner.cpp"
)
target_include_directories(SoftRendererTests PRIVATE "${SR_RENDERER_DIR}")
add_test(NAME SoftRenderer.TileLayout COMMAND SoftRendererTests)

if(NOT EXISTS "${SR_MEDIA_DIR}")
    message(FATAL_ERROR "Runtime media directory was not found: ${SR_MEDIA_DIR}")
endif()

add_custom_command(TARGET SoftRenderer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${SR_MEDIA_DIR}"
        "$<TARGET_FILE_DIR:SoftRenderer>/media"
    COMMENT "Copying runtime media"
)

set_property(TARGET SoftRenderer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:SoftRenderer>")

install(TARGETS SoftRenderer RUNTIME DESTINATION .)
install(TARGETS SrSwRendererCpu RUNTIME DESTINATION renderer)
install(TARGETS BasicShaders AdvanceShaders RUNTIME DESTINATION shader)
install(DIRECTORY "${SR_MEDIA_DIR}/" DESTINATION media)
