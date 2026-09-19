cmake_minimum_required(VERSION 3.24)
project(Lev4k LANGUAGES CXX)

if(NOT WIN32 OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "This host currently requires Windows and MSVC. CMake does not yet provide a Linux port.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Crinkler requires x86: use the windows-x86 preset, or configure Visual Studio with -A Win32.")
endif()
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Use a separate build directory, for example cmake --preset windows-x86.")
endif()

set(LEV4K_MAX_INTRO_BYTES 4095 CACHE STRING "Maximum intro size in bytes (4095 means strictly less than 4 KiB)")
if(NOT LEV4K_MAX_INTRO_BYTES MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "LEV4K_MAX_INTRO_BYTES must be a positive integer.")
endif()
option(LEV4K_BUILD_GPU_TESTS "Build the hidden-window GPU regression probes" OFF)
set(LEV4K_SHADER_MINIFIER "${CMAKE_CURRENT_SOURCE_DIR}/shader_minifier.exe" CACHE FILEPATH "Shader Minifier executable")
set(LEV4K_CRINKLER "${CMAKE_CURRENT_SOURCE_DIR}/link.exe" CACHE FILEPATH "Crinkler executable")
foreach(tool IN ITEMS LEV4K_SHADER_MINIFIER LEV4K_CRINKLER)
    if(NOT EXISTS "${${tool}}")
        message(FATAL_ERROR "Missing ${tool}: ${${tool}}")
    endif()
endforeach()

# These targets use explicit Editor/tiny profiles, independently of the IDE's
# configuration name. Avoid CMake's default /O2, /RTC1, /MDd and /EHsc leaking
# into the tiny objects. Runtime selection remains a per-target property.
set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W3")
foreach(config IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_CXX_FLAGS_${config} "")
endforeach()

set(shader_header "${CMAKE_CURRENT_BINARY_DIR}/generated/fragment.inl")
add_custom_command(
    OUTPUT "${shader_header}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${LEV4K_SHADER_MINIFIER}" --preserve-externals --no-renaming-list m1,m2,m3,m4
            -o "${shader_header}" "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/fragment.frag"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/fragment.frag" "${LEV4K_SHADER_MINIFIER}"
    COMMENT "Minifying GLSL into the build directory"
    VERBATIM
)
add_custom_target(lev4k_shaders DEPENDS "${shader_header}")

function(lev4k_profile target)
    add_dependencies(${target} lev4k_shaders)
    target_compile_definitions(${target} PRIVATE
        WINDOWS A32BITS SIMD _CRT_SECURE_NO_WARNINGS
        "LEV4K_SHADER_HEADER=\"${shader_header}\"")
    target_compile_options(${target} PRIVATE /O1 /Ob1 /Oi /Os /Oy /GS- /Gz /Zc:inline)
    set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreaded)
endfunction()

add_executable(lev4k_editor src/editor.cpp src/export.cpp src/main.cpp src/MidiIn.cpp src/song.cpp)
lev4k_profile(lev4k_editor)
target_compile_definitions(lev4k_editor PRIVATE EDITOR_CONTROLS DEBUG _DEBUG)
target_compile_options(lev4k_editor PRIVATE /EHsc /Zi)
target_link_libraries(lev4k_editor PRIVATE opengl32 winmm)
target_link_options(lev4k_editor PRIVATE /INCREMENTAL:NO /MANIFEST:NO /DYNAMICBASE:NO /DEBUG)
set_target_properties(lev4k_editor PROPERTIES
    OUTPUT_NAME Lev4k-editor
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/out/$<CONFIG>"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT lev4k_editor)

# Match the legacy project's object order and size-oriented compiler settings.
# Crinkler receives these objects directly, without an intermediate MSVC link.
add_library(lev4k_tiny_objects OBJECT EXCLUDE_FROM_ALL src/editor.cpp src/export.cpp src/main.cpp)
lev4k_profile(lev4k_tiny_objects)
target_compile_definitions(lev4k_tiny_objects PRIVATE NDEBUG)
target_compile_options(lev4k_tiny_objects PRIVATE /GF- /fp:fast /Zl /Gs)

# Custom commands need the matching SDK import-library environment even when
# CMake/MSBuild was launched from an ordinary PowerShell rather than a dev shell.
file(TO_CMAKE_PATH "${CMAKE_CXX_COMPILER}" compiler_path)
string(REGEX REPLACE "/Tools/MSVC/.*$" "" vc_root "${compiler_path}")
set(LEV4K_VCVARS "${vc_root}/Auxiliary/Build/vcvarsall.bat")
if(NOT EXISTS "${LEV4K_VCVARS}")
    message(FATAL_ERROR "Cannot locate vcvarsall.bat for ${CMAKE_CXX_COMPILER}.")
endif()
string(REGEX MATCH "/Tools/MSVC/([^/]+)/" unused "${compiler_path}")
set(LEV4K_VCTOOLS_VERSION "${CMAKE_MATCH_1}")
set(LEV4K_SDK_VERSION "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
if(NOT LEV4K_SDK_VERSION)
    string(REGEX REPLACE "[/\\]+$" "" LEV4K_SDK_VERSION "$ENV{WindowsSDKVersion}")
endif()
configure_file(cmake/run-crinkler.cmd.in run-crinkler.cmd @ONLY)

function(lev4k_intro name mode tries orders hash_size)
    set(output "${CMAKE_CURRENT_BINARY_DIR}/out/$<CONFIG>/Lev4k-${name}.exe")
    set(report "${CMAKE_CURRENT_BINARY_DIR}/out/$<CONFIG>/${name}-report.html")
    add_custom_command(
        OUTPUT "${output}"
        BYPRODUCTS "${report}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/out/$<CONFIG>"
        COMMAND "${CMAKE_CURRENT_BINARY_DIR}/run-crinkler.cmd"
            /CRINKLER /NOLOGO /OUT:${output} /REPORT:${report}
            /SUBSYSTEM:WINDOWS /ENTRY:entrypoint /NODEFAULTLIB /MACHINE:X86
            /RANGE:opengl32 /HASHTRIES:${tries} /COMPMODE:${mode} /ORDERTRIES:${orders}
            /HASHSIZE:${hash_size} /UNSAFEIMPORT /UNALIGNCODE /SATURATE
            /NOINITIALIZERS /TRANSFORM:CALLS /TRUNCATEFLOATS:32
            opengl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib
            "$<TARGET_OBJECTS:lev4k_tiny_objects>"
        DEPENDS lev4k_tiny_objects "$<TARGET_OBJECTS:lev4k_tiny_objects>"
                "${LEV4K_CRINKLER}" "${CMAKE_CURRENT_BINARY_DIR}/run-crinkler.cmd"
        COMMAND_EXPAND_LISTS VERBATIM
        COMMENT "Linking and compressing ${name} with Crinkler"
    )
    # Always check the limit, even if the EXE is already up to date or a previous
    # size check failed. Changing the limit must not require recompression.
    add_custom_target(lev4k_${name}
        COMMAND "${CMAKE_COMMAND}" "-DINTRO=${output}" "-DMAX_BYTES=${LEV4K_MAX_INTRO_BYTES}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckIntroSize.cmake"
        DEPENDS "${output}"
        VERBATIM)
endfunction()
lev4k_intro(snapshot INSTANT 100 2000 100)
lev4k_intro(release SLOW 300 9000 1000)

if(LEV4K_BUILD_GPU_TESTS)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/feedback-probe.cpp")
        message(FATAL_ERROR "The feedback probes require the full default-sample checkout, not a prepared production example.")
    endif()
    enable_testing()
    foreach(mipmaps IN ITEMS 0 1)
        set(probe lev4k_feedback_probe_${mipmaps})
        add_executable(${probe} tests/feedback-probe.cpp)
        add_dependencies(${probe} lev4k_shaders)
        target_compile_features(${probe} PRIVATE cxx_std_17)
        target_compile_options(${probe} PRIVATE /EHsc)
        target_compile_definitions(${probe} PRIVATE _CRT_SECURE_NO_WARNINGS USE_MIPMAPS=${mipmaps}
            "LEV4K_SHADER_HEADER=\"${shader_header}\"")
        target_link_libraries(${probe} PRIVATE opengl32 winmm gdi32 user32)
        add_test(NAME feedback_mipmaps_${mipmaps} COMMAND ${probe})
        set_tests_properties(feedback_mipmaps_${mipmaps} PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" RUN_SERIAL TRUE TIMEOUT 60)
    endforeach()
endif()
