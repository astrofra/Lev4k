# Lev4K feasibility study

Date: 19 September 2026. Scope: the `main` checkout at commit `1e3d1654f495d82b91063dd8d23da34135d6d431`, Windows 11, OpenGL, native Ubuntu, and introductory teaching material.

This page records the **original baseline assessment**. The Windows build maintenance, Editor reload repairs, and feedback rendering have since been implemented. See the [LastFrameBuffer guide](last-frame-buffer.md) for the current code, 1,724-byte Release, and GPU results.

## Recommendation

**Proceed with Windows 11 maintenance and a staged Ubuntu port.** Start by fixing the build settings and shader reload logic, establish a working audiovisual reference, and preserve a small Windows release target. Develop the Ubuntu teaching application with a normal runtime and diagnostics. Treat a Linux executable below the agreed byte limit as a separate milestone whose success must be measured.

The requested graphics conversion is already satisfied: the active renderer uses OpenGL, WGL, and GLSL. OpenGL modernization is useful for portability, but a Direct3D replacement project would solve the wrong problem.

| Objective | Feasibility | Confidence and condition |
| --- | --- | --- |
| Build with Visual Studio 2022 on Windows 11 x64 | High | Demonstrated after selecting SDK `10.0.22621.0`; Snapshot/Release additionally require default structure alignment. |
| Produce a Windows executable below 4,096 bytes | High for the supplied sample | Measured at 1,573 bytes in Release. Complete runtime behavior and future production content remain separate tests. |
| Run reliably across Windows 11 GPU configurations | Plausible, with maintenance | One NVIDIA driver passed shader compilation. Fullscreen, audio, reload, and other GPU drivers remain untested. |
| Use OpenGL | Already implemented | Confirmed directly in source and project dependencies. |
| Native Ubuntu teaching/editor application | High, with a platform port | No Linux target exists; windowing, audio, input, timing, and build integration must change. |
| Native Ubuntu production below the same byte limit | Conditional | ELF packaging, dependency policy, compression, and selected architecture need a measured prototype. |
| Teach beginners | Suitable with instructor preparation | Students can change effects quickly; low-level framework maintenance requires more experience. |

See the [validation record](validation-record.md) for the distinction between compilation, shader tests, and outstanding execution tests.

## What the repository contains

| Component | Current implementation | Evidence |
| --- | --- | --- |
| Window and operating-system integration | Win32 window, display mode switch, keyboard polling, minimal message handling | [main.cpp](../src/main.cpp), `main` / `entrypoint` |
| Graphics | Legacy WGL context, dynamically resolved OpenGL functions, fullscreen rectangles | [main.cpp](../src/main.cpp), [gldefs.h](../src/gldefs.h) |
| Visual content | GLSL `#version 330`; `m1` scene and `m2` post-process | [fragment.frag](../src/shaders/fragment.frag) |
| Default music | `m3` generates floating-point samples on the GPU; WinMM `waveOut*` plays the buffer | [Audio_Shaudio.h](../src/Audio_Shaudio.h), [shaudio.h](../src/shaudio.h) |
| Optional audio-file player | DirectShow/COM, used by the `Song` class | [song.h](../src/song.h), [song.cpp](../src/song.cpp) |
| Development tools | Shader reload, time navigation, camera state, image/audio export, optional MIDI | [debug.h](../src/debug.h), [editor.cpp](../src/editor.cpp), [export.cpp](../src/export.cpp) |
| Build | Visual Studio solution, MSVC `v143`, Windows SDK `10.0.17763.0`, Win32 only | [Lev4k.vcxproj](../Lev4k.vcxproj), [Lev4k.sln](../Lev4k.sln) |
| Size reduction | Bundled Shader Minifier 1.3.5 and Crinkler 2.1a, named `link.exe` | Local tool output; [Crinkler manual](../crinkler-manual.txt) |

There is no CMake project, Linux platform implementation, or native ELF build in this checkout. This assessment covers `main`; the separate [upstream branch review](branch-review.md) examines the other seven branches and their potential for reuse.

## Windows 11

### Architecture and tools

Keep the production target **x86 / Win32** initially. Windows x64 supports 32-bit applications through WOW64; 64-bit Windows does not require a 64-bit executable. Crinkler's supported output is small 32-bit Windows executables. Windows on ARM is a separate, untested target for this study. [Microsoft WOW64 documentation](https://learn.microsoft.com/en-us/windows/win32/winprog64/running-32-bit-applications), [Crinkler upstream](https://github.com/runestubbe/Crinkler).

Use Visual Studio 2022 with the C++ desktop tools and an installed Windows SDK. The project already selects `v143`, the VS 2022 toolset. A Windows SDK version and the installed operating-system version need not have matching numbers. [Microsoft toolset documentation](https://learn.microsoft.com/en-us/cpp/build/how-to-modify-the-target-framework-and-platform-toolset?view=msvc-170).

Observed build results:

| Trial | Result |
| --- | --- |
| Unchanged SDK selection | `MSB8036`: SDK `10.0.17763.0` is absent on this machine. |
| Editor with SDK `10.0.22621.0` | Success, including Shader Minifier. |
| Snapshot with that SDK, original `/Zp1` packing | `C2338` in Windows headers: default packing required. |
| Snapshot with default packing | Success; compression log reports 1,614 bytes. |
| Release with default packing | Success; file length is 1,573 bytes. |

All experiments used a temporary copy. `/PROGRESSGUI` was removed there for unattended compression. The original project was left unchanged during those baseline experiments.

### Maintenance identified in the baseline

The following maintenance is now included in the LastFrameBuffer integration:

1. Retarget all configurations to an installed SDK; retain `v143` and Win32.
2. Set **C/C++ > Code Generation > Struct Member Alignment > Default** for Snapshot and Release. Do not suppress the SDK packing assertion.
3. Correct the shader pass selector before teaching or judging editor runtime behavior; see the [OpenGL assessment](opengl-assessment.md).
4. Make the shader minification command use explicit quoted project-relative paths and remove its trailing unmatched quotation mark. The present command succeeded locally, so this is build hygiene, not the observed blocker.
5. Ensure the repository's Crinkler `link.exe` is selected for Snapshot/Release. Editor delegates to the normal MSVC linker; the logs should show this distinction.

The compiler also reports `/QIfist` as an unknown option. Remove it in a maintenance change and remeasure; it was ignored in the recorded trials.

### Runtime work before claiming compatibility

The current code does not check window creation, pixel format selection, context creation, most graphics calls, or audio opening. Add useful diagnostics to Editor, including actual GL vendor/version/profile, required functions, shader **link** results, framebuffer completeness, and audio return codes.

The source combines a GLSL 3.30 shader with `glCreateShaderProgramv`, an OpenGL 4.1-era API also available through `GL_ARB_separate_shader_objects`, and legacy drawing operations. A practical initial target is **OpenGL 4.1 or later with compatibility functionality**. Merely advertising OpenGL 3.3 or obtaining a non-null function pointer is insufficient. [Khronos separate shader objects specification](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_separate_shader_objects.txt), [Khronos OpenGL 4.1 reference card](https://www.khronos.org/files/opengl41-quick-reference-card.pdf).

The initial WGL context has no explicit version/profile request. GPU-driver support therefore matters. The tested NVIDIA driver reports OpenGL 4.6; this does not certify Intel, AMD, a virtual machine, or a remote session.

Fullscreen currently changes display settings, hides the cursor, and uses a numeric window-class atom. DPI awareness is commented out. Prefer a named/registered window class and borderless presentation in the teaching application, handle close events properly, and restore display/cursor state during cleanup. Test 100%, 125%, and 150% scaling, Escape, Alt+Tab, and multiple monitors. The message loop currently removes one message without translating or dispatching it.

### Compression tools

The bundled Crinkler runs on the tested Windows 11 installation. Its 2019 manual cannot establish compatibility with every later system. Evaluate a newer version separately, with file-size and runtime comparisons. Upstream Crinkler 3.x introduces SSE4.2 requirements for both the tool and generated executables; updating it changes the supported CPU baseline. [Crinkler release notes](https://github.com/runestubbe/Crinkler/releases).

The project uses `/UNSAFEIMPORT` and `/NOINITIALIZERS`. The first removes missing-DLL checks; the second suppresses dynamic C++ initializers. These are deliberate size choices, and explain why normal application assumptions about global constructors or diagnostics may fail. See the corresponding sections of the [bundled manual](../crinkler-manual.txt).

## The 4K constraint

Source text, object size, compressed executable size, and runtime memory are different quantities. Shader Minifier reduces embedded GLSL; Crinkler links and compresses the Windows program. Measure the resulting file after every meaningful content or platform change.

The recorded 1,573-byte Release leaves **2,523 bytes below a 4,096-byte inclusive cap**. This is arithmetic headroom, not a prediction of how much new source fits: compression changes globally as content changes. A strict less-than-4,096 requirement leaves 2,522 usable additional bytes.

The default 145-second music uses 44,100 stereo frames per second:

```text
Song frames                 = 44,100 * 145       = 6,394,500
Allocated audio pixel slots = 1,920 * 3,334      = 6,401,280
CPU stereo float buffer     = slots * 2 * 4     = 51,210,240 bytes
GPU RGBA32F texture storage  = slots * 4 * 4     = 102,420,480 bytes
```

Those two allocations alone are about **146.5 MiB**, before graphics buffers and driver overhead. The music texture uses the four-channel internal format `GL_RGBA32F`, while the CPU buffer stores two audio channels. Large generated buffers can coexist with a tiny executable because their sample values are produced at runtime. The allocation is in [Audio_Shaudio.h](../src/Audio_Shaudio.h); dimensions and duration are in [shaudio.h](../src/shaudio.h).

## Teaching suitability

Use the framework to teach the relationship between a C++ host, GPU shaders, procedural content, and executable size. Begin with a color gradient, animation, and a simple distance field. Add sound and size optimization after students can explain the render loop.

Students need variables, functions, loops, arrays, basic pointers, and simple vector mathematics. GLSL looks similar to C++, but it is a separate language compiled by the graphics driver. The compact release startup, writable shader assumptions, preprocessor tricks, and omission of the normal runtime should be instructor-led topics.

Keep the development application readable and diagnosable. Do not require beginners to remove error handling or learn custom executable formats as their first exercise. The [beginner guide](beginner-guide.md) provides a concrete progression.

## Proposed delivery plan

Estimates below are engineering judgment for one developer familiar with C++ and graphics, not measured durations or a student timetable. They include basic testing and exclude production artwork/music.

| Stage | Work | Estimate | Exit condition |
| --- | --- | --- | --- |
| 1 | Windows settings, shader selector, diagnostics, initial runtime reference | 2–4 working days | Correct editor reload, audio, and compressed executable on a reference PC. |
| 2 | Shared renderer boundary, explicit context contract, optional core-profile implementation | 3–6 days | Matching reference images and audio samples; documented Windows size change. |
| 3 | Ubuntu SDL2 host, audio clock, input, CMake, native shader generation | 5–10 days | Native editor runs the complete sample on the selected Ubuntu baseline. |
| 4 | Cross-driver, X11/Wayland, scaling, pause/seek, and packaging tests | 3–5 days | Acceptance matrix passes on declared targets. |
| 5 | Linux size prototype and production optimization | 5–15 days initially | A measured payload meets the agreed rules, or a documented size barrier stops this track. |

Stages 2 and 3 can share implementation work. Stages 1–4 are roughly **13–25 working days** if performed sequentially at these estimates; the optional Linux size track has substantially more uncertainty. Optional synths, MIDI, and export repairs add scope.

Proceed to classroom use only after the Windows runtime gates pass. Proceed to a native Ubuntu 4K commitment only after a minimal visual-and-audio prototype fits the byte limit on the intended judging setup. Detailed acceptance criteria are in the [validation record](validation-record.md).
