# Getting started with Lev4K

This guide is for students who know basic C++ variables, functions, conditions, and loops. It introduces this particular framework, rather than assuming that its very small release executable is a model for every C++ application.

**Start on Windows with the prepared Editor build.** Native Ubuntu support is proposed in the [Ubuntu study](ubuntu-porting-study.md); it is not implemented in this checkout. An instructor should apply and verify the preparation steps below before a class.

## 1. What are we making?

A demoscene intro is a program that generates an audiovisual experience. In a size-limited intro, formulas and small pieces of code replace large pictures, models, and recorded audio files.

Lev4K has two main languages:

- **C++ runs on the CPU.** It creates the window, communicates with the graphics driver, plays generated audio, and controls time.
- **GLSL runs on the GPU.** It computes image colors and, in this sample, audio samples. GLSL resembles C++, but it has different types, built-in functions, and execution rules.

OpenGL is the API through which the C++ program asks the GPU to work. This version already uses it.

The 4K target concerns the final file delivered to the competition. The source, development tools, and runtime memory are much larger. Check the exact rule: 4,096 bytes, strictly less than 4,096, and 4,000 bytes are different limits.

## 2. Prepare the Windows project

Install Visual Studio 2022 with **Desktop development with C++**, the MSVC v143 x86/x64 tools, and a Windows SDK. Use the graphics driver appropriate to your GPU. The measured build used SDK `10.0.22621.0`; another installed SDK needs its own verification.

Open [Lev4k.sln](../Lev4k.sln). In the solution toolbar choose **Editor** and **x86**. The project calls the same architecture **Win32**; these names are consistent here. Do not select x64 for this existing solution.

Before the first run, an instructor or maintainer should make these changes in a working copy:

1. Open project properties for **All Configurations / Win32**, and select an installed **Windows SDK Version** under General. The checked-in value, `10.0.17763.0`, may be missing.
2. For **Snapshot** and **Release**, choose **C/C++ > Code Generation > Struct Member Alignment > Default**. The checked-in 1-byte alignment fails with the tested modern SDK.
3. Apply the marker-based editor selector fix in the [OpenGL assessment](opengl-assessment.md). Without it, this LF checkout's post-process and music programs fail to link even though C++ compilation succeeds.
4. For Editor, set **Debugging > Working Directory** to `$(ProjectDir)`. The shader loader opens `./src/shaders/fragment.frag` relative to the process's working directory.
5. Keep the repository's shader minifier and Crinkler `link.exe` accessible to the build. Do not replace the Microsoft installation's linker with the bundled file.

For Editor, Snapshot, and Release, a clearer **Build Events > Pre-Build Event > Command Line** is:

```text
"$(ProjectDir)shader_minifier.exe" -v --preserve-externals --no-renaming-list m1,m2,m3,m4 -o "$(ProjectDir)src\shaders\fragment.inl" "$(ProjectDir)src\shaders\fragment.frag"
```

The checked-in command has a trailing unmatched quote, although it happened to run successfully in the recorded build. Keep `EditorNoRecompile`'s event empty: that configuration intentionally skips this generation step.

Build with **Build > Build Solution**, then run with **F5**. A successful C++ build is the first checkpoint; correct pictures, sound, and reload are the next checkpoints. This study verified compilation and shader linking, but did not validate a complete interactive session. Press **Escape** to exit a running intro. Expect the initial music calculation to take some time.

### Optional command-line builds

From a Visual Studio 2022 Developer PowerShell, in the repository root, after the preparation above:

```powershell
msbuild .\Lev4k.sln /t:Rebuild /p:Configuration=Editor /p:Platform=x86 /p:WindowsTargetPlatformVersion=10.0.22621.0
```

Replace the SDK number if your prepared project uses a different installed version. Do not run different configurations concurrently: some intermediate directories are shared.

## 3. Find the files that matter

| File | Role | First thing to look for |
| --- | --- | --- |
| [main.cpp](../src/main.cpp) | C++ setup and frame loop | `AUDIO_TYPE`, window creation, `AudioInit`, `glUseProgram`, `SwapBuffers` |
| [definitions.h](../src/definitions.h) | Display and audio data structures | `XRES`, `YRES` |
| [fragment.frag](../src/shaders/fragment.frag) | Human-readable GPU code | `m1`, `m2`, `m3` |
| [fragment.inl](../src/shaders/fragment.inl) | Generated C++ text containing the minified shader | Read it for comparison; edit `.frag` instead. |
| [shaudio.h](../src/shaudio.h) | Music duration, sample rate, and render dimensions | `SONG_DURATION`, `SAMPLE_RATE`, `SHAUDIO_XRES`, `SHAUDIO_YRES` |
| [Audio_Shaudio.h](../src/Audio_Shaudio.h) | GPU music generation and Windows playback | `RenderMusic`, `AudioGetTime` |
| [debug.h](../src/debug.h) | Editor shader loading and debugging | `refreshShaders` |
| [editor.cpp](../src/editor.cpp) | Editor controls and statistics | Camera and timing functions |

A `.cpp` file is compiled as a C++ translation unit. A header such as `.h` or this project's `.inl` contributes text where `#include` inserts it. The compiler creates object files; the linker combines the required code and data into an executable.

The build has a second language pipeline:

```text
fragment.frag -> Shader Minifier -> fragment.inl -> C++ compiler
                                                    |
                                              object files
                                                    |
                                      linker / Crinkler -> executable
```

At runtime, the OpenGL driver compiles GLSL. Shader Minifier shortens shader source; it does not produce the final GPU machine code. In Editor, the program normally loads the readable `.frag` directly for rapid changes.

## 4. Read the render loop

After initialization, the program repeatedly:

1. Processes some operating-system/input work.
2. Gets the current audio position.
3. Sends that position to the scene shader.
4. Draws the scene, copies it into a texture, and draws the post-process.
5. Displays the new frame with `SwapBuffers`.

`pidMain` and `pidPost` are integer handles identifying GPU programs. `glUseProgram(pidMain)` selects a program. `glGetUniformLocation(pidMain, "m")` finds its time input, and `glUniform1i` supplies an integer value.

A **uniform** has the same value for every fragment in a draw. `gl_FragCoord` describes the current fragment's screen position. The shader's `out vec4 o1` writes four values: red, green, blue, and alpha.

The functions have these roles:

| Function | Work |
| --- | --- |
| `m1` | Draws the main image. The sample uses raymarching through a mathematical distance field. |
| `m2` | Reads texture `sb1` and processes the image. The sample offsets the color channels. |
| `m3` | Produces music samples in a texture instead of visible pixels. |
| `m4` | Reserved for optional audio post-processing; absent from the supplied shader. |

The host changes `#define m1 main` to select each pass, then compiles another program. Keep the first two shader lines and pass names intact while learning.

## 5. First exercise: an animated gradient

Back up or commit your starting shader. In `src/shaders/fragment.frag`, replace the complete `m1` and `m2` function definitions with the following. Keep the other declarations and `m3`:

```glsl
void m1(void)
{
    float seconds = float(m) / 44100.0;
    vec2 uv = gl_FragCoord.xy / res;
    float blue = 0.5 + 0.5 * sin(seconds);
    o1 = vec4(uv.x, uv.y, blue, 1.0);
}

void m2(void)
{
    vec2 uv = gl_FragCoord.xy / res;
    o1 = texture(sb1, uv);
}
```

`vec2` contains two numbers; `vec4` contains four. Here `uv` expresses screen position on an approximately 0-to-1 scale. The red channel grows horizontally, the green channel grows vertically, and the blue channel changes with time. `sin` returns values between -1 and 1; scaling and shifting maps them into the color range 0 to 1.

Save the file and trigger the prepared editor's **Ctrl+S** reload. Its current implementation polls the keyboard rather than watching file changes; a rebuild/restart is another way to load the saved shader. Music is regenerated by default on reload, so a pause may occur.

Try changing `sin(seconds)` to `sin(seconds * 2.0)`. The animation should run twice as fast. Then restore the original `m1` and change the object color or rotation speed. Change one expression at a time so that you can explain the result.

If the window is black, inspect shader errors and the known selector issue before rewriting the effect. The current debug helper does not check every pass's link status.

## 6. Time, sound, and resolution

The default uniform `m` represents **audio sample frames**, not milliseconds. At 44,100 frames per second:

```text
m =      0  -> 0 seconds
m = 44,100  -> 1 second
m = 88,200  -> 2 seconds
```

A stereo frame contains a left and right sample. In C++, the buffer therefore needs two `float` values per frame. Dividing the frame count by 44,100 converts it to seconds.

To change duration, edit `SONG_DURATION` in `shaudio.h` and satisfy:

```text
SHAUDIO_XRES * SHAUDIO_YRES >= SAMPLE_RATE * SONG_DURATION
```

For 145 seconds at the default width, the minimum height is `ceil(44100 * 145 / 1920) = 3331`; the supplied height is 3334. More height means more generated samples and more memory. The static assertion in `definitions.h` catches insufficient capacity. Duration changes require rebuilding C++, not just shader reload.

For visual resolution, change both `XRES`/`YRES` in `definitions.h` and `res` in `fragment.frag`. The shader-audio width is separate: `m3` hard-codes 1920 in its sample index calculation. If you change `SHAUDIO_XRES`, update that expression too. Changing only the visual resolution does not require changing audio dimensions.

The default sound path is `AUDIO_SHAUDIO`. Do not assume other backends work by changing one macro. The main-branch WAV adapter has a local `track` variable used outside its scope; optional synths need build/link integration. Reverb is also incomplete here: enabling it requires an `m4` implementation and additional code repairs.

## 7. Editor controls in this checkout

These are derived from the actual code, which differs from the upstream README:

| Keys | Behavior |
| --- | --- |
| Escape | Exit. |
| Ctrl+S | Reload shaders; regenerate music when `SHAUDIO_UPDATEONSAVE` is 1. |
| Alt+Up | Play. |
| Alt+Down | Pause. |
| Alt+Space | Seek to the beginning; it does not independently force playback on. |
| Alt+Left / Alt+Right | Seek backward/forward by roughly one second per polling iteration. |
| Alt+Shift+Left / Right | Finer seeking: roughly 0.1 second per polling iteration. |

Holding a key can repeat the action each rendered frame. Shift makes these steps **smaller**, contrary to the old README. Camera controls update C++ state, but the supplied shader comments out the camera uniforms and uses a fixed camera; movement becomes visible only after wiring those inputs into the shader.

## 8. Build a small release

| Configuration | Purpose | Output |
| --- | --- | --- |
| `Editor` | Diagnostics/controls, raw-shader reload, pre-build minification | `out/Lev4k-debug.exe` |
| `EditorNoRecompile` | Same editor code, skips pre-build minification | `out/Lev4k-debug.exe` |
| `Snapshot` | Faster compressed build using embedded shader | `out/Lev4k-snapshot.exe` |
| `Release` | More compression effort and HTML report | `out/Lev4k-release.exe`, `out.html` |

`EditorNoRecompile` still compiles C++ when needed and still reloads the raw shader. Snapshot and Release have different output names in this project, even though they share an intermediate directory. Rebuild when switching them; do not rely on the older README's claim that Snapshot simply overwrites the Release executable.

After testing your effect in Editor, rebuild Snapshot, then Release. From Developer PowerShell:

```powershell
msbuild .\Lev4k.sln /t:Rebuild /p:Configuration=Release /p:Platform=x86 /p:WindowsTargetPlatformVersion=10.0.22621.0
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
$introBytes = (Get-Item -LiteralPath .\out\Lev4k-release.exe).Length
Write-Output "Release size: $introBytes bytes"
if ($introBytes -ge 4096) { throw 'Strictly less than 4096 bytes is required' }
```

This example deliberately enforces **strictly less than 4,096**. For a competition allowing exactly 4,096, change the check to `-gt 4096`; for a strict 4,000-byte limit, use the corresponding threshold. Count every required submitted file under the actual rules.

Check `out.html` to see what consumes compressed space. Make one change, rebuild, record the new byte count, and compare appearance/sound. Shorter source is not always a smaller compressed executable.

The sample measured 1,573 bytes under the study's conditions. Your content, compiler, and linker can produce different results. Test the exact Release file on the intended playback machine; Editor success is not Release acceptance.

## 9. Common problems

| Symptom | What to check |
| --- | --- |
| `MSB8036` | Select an installed Windows SDK for all configurations. |
| `C2338` mentioning packing | Use default structure alignment in Snapshot/Release. |
| Black screen or missing music despite a successful build | Apply the selector repair; check all shader link logs, context support, and audio opening. |
| Shader edits seem ignored | Save `.frag`, use the repository root as working directory, reload or restart; rebuild embedded-shader configurations. |
| Unknown `/CRINKLER` or MSVC linker-option warnings | Check whether the repository's `link.exe` is selected for the compressed configurations. |
| Wrong image size/aspect | Keep C++ visual dimensions and GLSL `res` consistent; check desktop scaling. |
| Slow startup/reload | Music renders the entire buffer; simplify `m3` or test a shorter duration with valid dimensions. |
| Missing optional synth symbol | Build/link the required synth objects and review the relevant branch's integration; default shader audio needs none. |

For normal C++ coursework, continue using standard library containers, clear ownership, and normal initialization. Lev4K's tiny Release removes normal startup/runtime facilities and uses aggressive size settings. Learn what each shortcut costs before using it in your own release.

## 10. Suggested learning sequence

1. Explain the C++/GLSL split and build the prepared Editor.
2. Implement and explain the gradient above.
3. Animate a parameter using audio time.
4. Restore the raymarcher and change one shape or color parameter.
5. Add a simple post-process and compare its byte cost.
6. Change a music parameter, observe duration/memory constraints, and compare sound.
7. Produce a measured release with a short compatibility record.

Ubuntu students can study the same shader exercises after the native host is implemented. Until then, the [Ubuntu porting study](ubuntu-porting-study.md) is an implementation plan, not an installation guide for a working Linux Lev4K release.
