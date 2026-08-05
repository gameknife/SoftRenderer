cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(UWP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(SR_ROOT "${UWP_ROOT}/..")
set(SR_CODE_DIR "${SR_ROOT}/code")
set(SR_CORE_DIR "${SR_CODE_DIR}/SoftRenderer")
set(SR_RENDERER_DIR "${SR_CODE_DIR}/SrSwRendererCpu")

if(NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    message(FATAL_ERROR
        "The SoftRendererUwp target requires CMAKE_SYSTEM_NAME=WindowsStore.")
endif()

get_filename_component(_program_files_x86
    "$ENV{ProgramFiles}/../Program Files (x86)"
    ABSOLUTE
)
set(_windows_sdk_shader_path
    "${_program_files_x86}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64"
)
find_program(FXC_EXECUTABLE
    NAMES fxc.exe
    HINTS
        "${_windows_sdk_shader_path}"
        "$ENV{WindowsSdkDir}/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64"
)
if(NOT FXC_EXECUTABLE)
    message(FATAL_ERROR
        "fxc.exe was not found. Install the Windows SDK shader compiler.")
endif()

set(SHADER_SOURCE "${UWP_ROOT}/shaders/Frame.hlsl")
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")
set(VERTEX_SHADER "${SHADER_OUTPUT_DIR}/FrameVS.cso")
set(PIXEL_SHADER "${SHADER_OUTPUT_DIR}/FramePS.cso")
set(SHADER_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/SoftRendererUwpShaders.h")
set(SHADER_EMBED_SCRIPT "${UWP_ROOT}/scripts/EmbedShader.ps1")

file(GLOB_RECURSE APP_MEDIA CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${SR_ROOT}/bin/media/*"
)
file(GLOB APP_ASSETS CONFIGURE_DEPENDS "${UWP_ROOT}/Assets/*.png")

set(UWP_CORE_SOURCES
    "${SR_CORE_DIR}/mmgr/mmgr.cpp"
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

set(UWP_RENDERER_SOURCES
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

set(APP_SOURCES
    "${UWP_ROOT}/src/App.cpp"
    "${UWP_ROOT}/src/App.h"
    "${UWP_ROOT}/src/App.idl"
    "${UWP_ROOT}/src/App.xaml"
    "${UWP_ROOT}/src/Main.cpp"
    "${UWP_ROOT}/src/MainPage.cpp"
    "${UWP_ROOT}/src/MainPage.h"
    "${UWP_ROOT}/src/MainPage.idl"
    "${UWP_ROOT}/src/MainPage.xaml"
    "${UWP_ROOT}/src/pch.cpp"
    "${UWP_ROOT}/src/pch.h"
    "${UWP_ROOT}/src/Renderer.cpp"
    "${UWP_ROOT}/src/Renderer.h"
    "${UWP_ROOT}/src/SoftRendererRuntime.cpp"
    "${UWP_ROOT}/src/SoftRendererRuntime.h"
    "${UWP_ROOT}/src/XamlGenerated.cpp"
    "${SR_CORE_DIR}/InputManager.h"
    "${SR_CORE_DIR}/RendererBase.h"
    "${SR_CORE_DIR}/prerequisite.h"
    "${UWP_ROOT}/../code/SoftRenderer/UwpInputManager.cpp"
    "${SR_CODE_DIR}/BasicShaders/BasicShaders.cpp"
    "${SR_CODE_DIR}/BasicShaders/BasicShaders.h"
    "${SR_CODE_DIR}/AdvanceShaders/AdvanceShaders.cpp"
    "${SR_CODE_DIR}/AdvanceShaders/AdvanceShaders.h"
    "${SR_CORE_DIR}/SrAppFramework.h"
    "${SR_CORE_DIR}/SrRenderContext.h"
    "${UWP_ROOT}/Package.appxmanifest"
    "${SHADER_HEADER}"
    ${APP_ASSETS}
    ${APP_MEDIA}
    ${UWP_CORE_SOURCES}
    ${UWP_RENDERER_SOURCES}
)

add_custom_command(
    OUTPUT "${SHADER_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUTPUT_DIR}"
    COMMAND "${FXC_EXECUTABLE}" /nologo /E FrameVS /T vs_5_1
        /Fo "${VERTEX_SHADER}" "${SHADER_SOURCE}"
    COMMAND "${FXC_EXECUTABLE}" /nologo /E FramePS /T ps_5_1
        /Fo "${PIXEL_SHADER}" "${SHADER_SOURCE}"
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
        -File "${SHADER_EMBED_SCRIPT}"
        -VertexShader "${VERTEX_SHADER}"
        -PixelShader "${PIXEL_SHADER}"
        -OutputHeader "${SHADER_HEADER}"
    DEPENDS "${SHADER_SOURCE}" "${SHADER_EMBED_SCRIPT}"
    COMMENT "Compiling and embedding UWP frame shaders"
    VERBATIM
)

add_executable(SoftRendererUwp WIN32 ${APP_SOURCES})

target_include_directories(SoftRendererUwp PRIVATE
    "${UWP_ROOT}/src"
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    "${SR_CORE_DIR}"
    "${SR_RENDERER_DIR}"
    "${SR_CODE_DIR}/BasicShaders"
    "${SR_CODE_DIR}/AdvanceShaders"
)

target_link_libraries(SoftRendererUwp PRIVATE d3d12.lib dxgi.lib)
target_compile_definitions(SoftRendererUwp PRIVATE
    WIN32_LEAN_AND_MEAN
    WINRT_LEAN_AND_MEAN
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS
    SR_STATIC_BUILD
    SR_UWP
    SR_USE_SIMD
)
target_compile_options(SoftRendererUwp PRIVATE /bigobj)

set_target_properties(SoftRendererUwp PROPERTIES
    VS_GLOBAL_AppContainerApplication "true"
    VS_GLOBAL_ApplicationType "Windows Store"
    VS_GLOBAL_ApplicationTypeRevision "10.0"
    VS_GLOBAL_CppWinRTGenerateWindowsMetadata "true"
    VS_GLOBAL_CppWinRTMergedDir "${CMAKE_CURRENT_BINARY_DIR}/SoftRendererUwp.dir/$(Configuration)/Merged/"
    VS_GLOBAL_CppWinRTOptimized "true"
    VS_GLOBAL_CppWinRTProjectLanguage "C++/WinRT"
    VS_GLOBAL_CppWinRTRootNamespaceAutoMerge "true"
    VS_GLOBAL_CppWinRTUnmergedDir "${CMAKE_CURRENT_BINARY_DIR}/SoftRendererUwp.dir/$(Configuration)/Unmerged/"
    VS_GLOBAL_CustomAfterMicrosoftCommonTargets "${UWP_ROOT}/CppWinRTOverrides.targets"
    VS_GLOBAL_DefaultLanguage "en-US"
    VS_GLOBAL_MinimalCoreWin "true"
    VS_GLOBAL_NuGetTargetMoniker "native,Version=v0.0"
    VS_GLOBAL_RootNamespace "SoftRendererUwp"
    VS_GLOBAL_TargetFramework "UAP,Version=v10.0"
    VS_GLOBAL_TargetFrameworks "UAP,Version=v10.0"
    VS_GLOBAL_WindowsTargetPlatformMinVersion "10.0.10240.0"
    VS_GLOBAL_AppxPackageSigningEnabled "false"
)

set_source_files_properties("${UWP_ROOT}/src/App.xaml" PROPERTIES
    VS_XAML_TYPE ApplicationDefinition
)
set_source_files_properties("${UWP_ROOT}/src/MainPage.xaml" PROPERTIES
    VS_XAML_TYPE Page
)
set_source_files_properties(
    "${UWP_ROOT}/src/App.idl"
    "${UWP_ROOT}/src/MainPage.idl"
    PROPERTIES VS_TOOL_OVERRIDE Midl
)
set_source_files_properties("${UWP_ROOT}/Package.appxmanifest" PROPERTIES
    VS_TOOL_OVERRIDE AppxManifest
)
set_source_files_properties(${APP_ASSETS} PROPERTIES
    VS_DEPLOYMENT_CONTENT 1
    VS_DEPLOYMENT_LOCATION Assets
)

foreach(media_file IN LISTS APP_MEDIA)
    file(RELATIVE_PATH media_relative "${SR_ROOT}/bin/media" "${media_file}")
    get_filename_component(media_directory "${media_relative}" DIRECTORY)
    if(media_directory STREQUAL "" OR media_directory STREQUAL ".")
        set(media_deployment_directory media)
    else()
        string(REPLACE "/" "\\" media_directory_windows "${media_directory}")
        set(media_deployment_directory "media\\${media_directory_windows}")
    endif()
    set_source_files_properties("${media_file}" PROPERTIES
        VS_TOOL_OVERRIDE None
        VS_DEPLOYMENT_CONTENT 1
        VS_DEPLOYMENT_LOCATION "${media_deployment_directory}"
    )
endforeach()

set_property(TARGET SoftRendererUwp PROPERTY
    VS_PACKAGE_REFERENCES "Microsoft.Windows.CppWinRT_2.0.220531.1"
)
