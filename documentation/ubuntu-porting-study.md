# Native Ubuntu porting study

## Decision and target

**A native Ubuntu port is feasible, but the current project does not build on Linux.** OpenGL and GLSL are reusable foundations; Win32/WGL, WinMM, DirectShow, MSVC-specific code, and the Windows executable pipeline are not portable as they stand.

Use **Ubuntu 24.04 LTS, amd64** as a concrete initial classroom baseline, then test the Ubuntu version selected for deployment as an additional target. This is a chosen baseline, not a claim that 24.04 is the newest release. Ubuntu's release-cycle page records the supported LTS lifetimes. [Ubuntu release cycle](https://ubuntu.com/about/release-cycle).

Separate two deliverables:

| Deliverable | Intended result | Size expectation |
| --- | --- | --- |
| Native development application | Editable shaders, diagnostics, normal window/input handling, and synchronized audio | A normal Linux executable with declared dependencies; no 4K claim. |
| Native production release | A reproducible ELF or other permitted payload for a specified competition environment | Must pass a separate exact-byte and dependency test. |

Wine can be useful for comparing a Windows release, but does not create a native Linux port. WSL graphics tests also do not substitute for testing a native Ubuntu desktop session. This study performed neither Linux compilation nor Linux runtime tests; no installed WSL distribution was listed locally.

## Recommended implementation

Use **SDL2 for the first portable host**, with CMake and a normal C++ runtime. SDL2 provides window/context creation, input events, and audio playback behind one platform interface. It is available in the chosen Ubuntu release. Explicitly request OpenGL; do not accidentally create a separate SDL rendering backend. [Ubuntu `libsdl2-dev` package](https://packages.ubuntu.com/noble/libsdl2-dev), [SDL2 context creation](https://wiki.libsdl.org/SDL2/SDL_GL_CreateContext).

The initial graphical prototype can request a compatibility context to reuse the original drawing. For the durable teaching application, prefer the OpenGL 3.3 core work described in the [OpenGL assessment](opengl-assessment.md). These are implementation alternatives, not configuration switches already provided by Lev4K.

| Option | Appropriate use | Tradeoff |
| --- | --- | --- |
| SDL2 + OpenGL + SDL audio | Recommended functional/teaching port | Simplifies several platform services; linked library and import costs need separate evaluation for a tiny release. |
| GLFW + OpenGL + a separate audio backend | An alternative if the course already uses GLFW | GLFW handles context/input, but audio and its timing still need another implementation. [GLFW context guide](https://www.glfw.org/docs/latest/context_guide.html). |
| X11/GLX + ALSA | Candidate experiment for a restricted tiny Linux target | More platform code; not a native Wayland path; audio buffering and underrun recovery are the application's responsibility. [ALSA PCM API](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html). |

Do not replace the compact Windows host with SDL2 merely to get Linux support. Share shader/effect logic and an audio-frame timeline, while allowing different platform/release entry points.

## Source-level porting map

| Current dependency | Location | Ubuntu adaptation |
| --- | --- | --- |
| `windows.h`, `HWND`, `HDC`, `DEVMODE`, pixel format descriptor | [definitions.h](../src/definitions.h), [main.cpp](../src/main.cpp) | Move Win32 structures into a Windows platform file. Use SDL window/context settings in the portable host. |
| `CreateWindow`, `ChangeDisplaySettings`, `ShowCursor` | `main.cpp` | SDL window creation, window/fullscreen state, cursor API. Prefer borderless presentation initially. |
| `wglCreateContext`, `wglMakeCurrent`, `SwapBuffers` | `main.cpp` | `SDL_GL_CreateContext`, `SDL_GL_MakeCurrent`, `SDL_GL_SwapWindow`. |
| `wglGetProcAddress` macros | [gldefs.h](../src/gldefs.h) | A small loader using `SDL_GL_GetProcAddress`; validate version/extensions and use correct function signatures. |
| `GetAsyncKeyState`, `PeekMessage`, cursor/focus polling | `main.cpp`, [editor.cpp](../src/editor.cpp) | SDL event loop and window-focused input state; support close requests and key press/release semantics. |
| `timeGetTime`, `Sleep` | `editor.cpp`, [debug.h](../src/debug.h) | SDL performance counter or `std::chrono::steady_clock`; avoid making audio timing depend on render rate. |
| `waveOut*`, `WAVEHDR`, `WAVEFORMATEX`, `MMTIME` | [Audio_Shaudio.h](../src/Audio_Shaudio.h), `definitions.h` | SDL stereo float playback and an explicit playback cursor, with buffering/latency accounted for. |
| DirectShow/COM `Song` | [song.h](../src/song.h), [song.cpp](../src/song.cpp) | Exclude from the first port. Add optional WAV decoding plus SDL playback later; isolate `editor.h` from `song.h`. |
| WinMM MIDI | [MidiIn.cpp](../src/MidiIn.cpp) | Disable in the first milestone; later add a selected Linux MIDI backend if required. |
| Win32 directory/file helpers in export | [export.cpp](../src/export.cpp) | Portable file/directory operations in the development target; keep exporter out of tiny releases. |
| `__forceinline`, `__cdecl`, `__int64`, MSVC pragmas | Multiple C++ files | Ordinary C++ types/functions or narrowly scoped compiler macros; retain Windows calling conventions only where needed. |
| Custom `entrypoint`, no default runtime libraries | Project and `main.cpp` | Normal `int main()` for the first port. Reconsider startup only in the Linux size experiment. |
| Production string-literal mutation and byte-offset selector | `main.cpp` | Writable arrays or separate shader source strings. Editor already selects passes by marker in `debug.h`. |
| `.sln`, `.vcxproj`, `.bat`, `.obj`, `.exe` tools | Build files and synth directories | CMake targets, native build commands, ELF objects, and an explicit shader-generation step. |

On X11 a non-null GL function pointer does not prove that the active context supports that function. Use version/extension tests too. [SDL2 function-loading guidance](https://wiki.libsdl.org/SDL2/SDL_GL_GetProcAddress).

### Audio and synchronization

Keep shader audio as the first music backend. Its synthesis code is GLSL and avoids an immediate assembly port. Render the whole music buffer, read it to RAM, then start playback only when samples are ready.

Define time in **stereo sample frames at the source rate**: one frame contains left and right samples. Preserve the shader contract `seconds = m / 44100.0`. Request float32, two channels, and 44,100 Hz; either keep that application-facing format through conversion or adapt explicitly to the negotiated format. SDL audio can convert when changes are disallowed at the application interface. [SDL2 audio-device documentation](https://wiki.libsdl.org/SDL2/SDL_OpenAudioDevice).

A callback should copy precomputed samples, zero-fill at the end, and update a cursor safely. It should not render OpenGL, allocate large buffers, compile shaders, or perform blocking disk access. Pause/seek/reload must coordinate with the audio thread and reset the timeline offset.

Queued playback is another option. Submitted bytes minus queued bytes estimates how much SDL has passed onward; it does **not** establish the exact sample currently audible at the hardware. Likewise, a callback's delivery cursor can lead audible playback. Account for buffering, calibrate the reference, and document the remaining synchronization tolerance. [SDL2 queued-audio size semantics](https://wiki.libsdl.org/SDL2/SDL_GetQueuedAudioSize).

The existing `AUDIO_NONE` clock adds `1/60` second per rendered frame. It is suitable for fixed-step image export; it is not a correct live clock on an arbitrary display/frame rate.

### Optional music backends

The supplied 4klang build uses `nasmw.exe -fwin32` to produce a Windows object. It needs object-format, symbol, calling-convention, and architecture work for Linux. An x86 object cannot simply be linked into an amd64 executable.

Oidos has useful non-Windows section definitions in [platform.inc](../src/Oidos/platform.inc). Its [public header](../src/Oidos/oidos.h) explicitly leaves Linux playback and timing to the application. However, Lev4K's `Audio_Oidos.h` calls the Windows-only playback functions. This is partial portability support in the synth, not an already working Linux integration. Neither optional synth was built in this study.

## Build and developer setup

These commands prepare an Ubuntu development machine; they do **not** make the present checkout buildable:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build pkg-config libsdl2-dev libgl-dev mesa-utils
pkg-config --modversion sdl2
glxinfo -B
```

The graphics development package is `libgl-dev`; `libsdl2-dev` supplies the SDL2 development interface. [Ubuntu GL package](https://packages.ubuntu.com/noble/libgl-dev), [Ubuntu SDL2 package](https://packages.ubuntu.com/noble/libsdl2-dev).

`glxinfo` reports the GLX/X11 path, including XWayland when applicable. The application must separately log the renderer, version, and profile of its own native Wayland/EGL context. A software renderer such as llvmpipe may be useful for correctness checks, but is not performance acceptance for the intended GPU.

The port should add CMake targets with explicit source lists: do not glob every `.cpp`, since that pulls in DirectShow and WinMM files. Use `find_package(OpenGL REQUIRED)` and an SDL2 package/imported target or pkg-config integration verified on the baseline. Keep generated headers in the build directory and declare the GLSL source and minifier as dependencies.

For the first bring-up, the checked-in `fragment.inl` can provide a fixed shader baseline, with writable storage handled correctly. Before student editing is supported, provide reproducible shader generation on Ubuntu. Pin a Shader Minifier version and use that release's supported Linux/.NET/Mono workflow; the bundled Windows `.exe` is not automatically a native Linux command. Compare generated shaders and pass entry points after any tool upgrade. [Shader Minifier upstream instructions](https://github.com/laurentlb/shader-minifier).

A possible future layout is shown below. **These files and targets do not exist yet.**

```text
CMakeLists.txt
src/platform/windows_tiny.cpp
src/platform/sdl_host.cpp
src/render/renderer.cpp
src/audio/shader_synth.cpp
src/audio/sdl_playback.cpp
src/shaders/fragment.frag
build/generated/fragment.inl
```

## X11, XWayland, and native Wayland

Treat the window-system backend as a recorded test condition. SDL can select available video backends, and `SDL_VIDEODRIVER` can request `x11` or `wayland`. Backend availability depends on the installed SDL build and session. [SDL video-driver selection](https://wiki.libsdl.org/SDL2/SDL_HINT_VIDEODRIVER).

First test a normal window on X11/XWayland, then native Wayland, and finally fullscreen. Handle framebuffer size independently of logical window size for high-DPI output. Avoid depending on global keyboard state, arbitrary global cursor positioning, or an exclusive display-mode switch. Such assumptions make the Win32 editor harder to port than the shaders.

## Reaching less than 4K on Linux

Crinkler is a Windows 32-bit compressing linker; it is not an ELF backend. Neither the original 1,573-byte sample nor the 1,724-byte [feedback version](last-frame-buffer.md) predicts Linux size. [Crinkler project scope](https://github.com/runestubbe/Crinkler).

Start with a stripped native executable and inspect its code, shader data, dynamic imports, ELF metadata, and startup overhead. Compiler optimization for size and dead-code removal are useful baselines, but neither `strip` nor `-Os` guarantees a sub-4K file. ELF tools such as `sstrip` can remove some metadata; they do not replace a complete compression/decompression design. [ELFkickers upstream](https://github.com/BR903/ELFkickers).

The size prototype should answer four questions before committing to a production port:

1. Does an amd64 executable with a window, one visual shader, procedural stereo audio, and clean exit fit the agreed limit after the chosen packaging?
2. Which system libraries are permitted and actually installed on the target? Dynamically linking SDL2 keeps its implementation outside the executable, but does not make that dependency universally allowed or present.
3. If a launcher/decompressor or appended payload is used, what is the **total counted file size**, including all required submitted files?
4. Does the exact artifact run on the target kernel, loader, display server, GPU driver, and audio setup without downloading anything?

Do not pursue fully static SDL/window/audio linking as the initial 4K strategy. If the functional host is too large, compare a narrower native host and a dedicated Linux executable-compression workflow. A 32-bit Linux alternative needs matching 32-bit graphics/audio libraries; prefer amd64 first and consider i386 only with a measured benefit and an agreed target environment.

For a future artifact, measure its real file length, for example `stat -c %s ./intro`. Record shared-library dependencies and test the exact packaged file. A ZIP size or the shader's character count is not a release-size result.

## Acceptance criteria

- Native build from a clean Ubuntu baseline with documented packages and pinned generation tools.
- Correct scene, post-process, and stereo audio; fixed-time comparisons against the Windows reference.
- Stable playback, pause/seek, reload, and exit; no accumulated drift beyond a declared tolerance.
- X11/XWayland and native Wayland results recorded separately, including actual GL renderer/profile.
- At least one Mesa-backed Intel/AMD machine and one NVIDIA machine tested where available.
- Separate recorded outcomes for functional correctness, performance, and exact release bytes.

The functional port is a reasonable engineering commitment. The Linux sub-4K release remains conditional until this final size-and-runtime gate passes.
