# Lev4K

Lev4K is a Windows framework for size-limited demoscene intros, based on [Leviathan 2.0](https://github.com/armak/Leviathan-2.0). It is intended to be adapted for each production.

The `main` branch contains a sample with **OpenGL rendering, GLSL raymarching, a post-processing pass, and GPU-generated stereo music** played through WinMM. Shader Minifier reduces the embedded shader source, and Crinkler links and compresses the production executable.

Start with the [beginner guide](documentation/beginner-guide.md). The [documentation index](documentation/README.md) also links the Windows 11 feasibility study, OpenGL assessment, Ubuntu porting plan, and validation results.

## Requirements

- Visual Studio 2022 with the C++ desktop tools and MSVC **v143**.
- A Windows SDK. The project selects `10.0.17763.0`; retarget it to an installed version if necessary. The documented Windows 11 builds used `10.0.22621.0`.
- The **x86** solution platform, which maps to **Win32** in the project. There is no x64 build configuration.
- An OpenGL driver supporting GLSL 3.30, compatibility rendering, and `glCreateShaderProgramv` through OpenGL 4.1 or `GL_ARB_separate_shader_objects`. The current renderer uses legacy drawing calls, so a core-only context is insufficient. See the [OpenGL assessment](documentation/opengl-assessment.md).

The repository includes Shader Minifier `1.3.5` and Crinkler `2.1a`. Crinkler is named `link.exe` and is selected through the project's executable search path. Keep it in the repository; do not replace the linker in the Visual Studio installation.

Native Linux support is not implemented. The [Ubuntu study](documentation/ubuntu-porting-study.md) describes the platform changes and a separate path toward a size-limited Linux release.

## Prepare the project

1. Open [Lev4k.sln](Lev4k.sln), select **Editor / x86**, and select an installed Windows SDK under the project's **General** properties for all configurations.
2. For **Snapshot** and **Release**, set **C/C++ > Code Generation > Struct Member Alignment** to **Default**. The checked-in 1-byte packing fails with the SDK used in the Windows 11 study.
3. Set the Editor debugging **Working Directory** to `$(ProjectDir)`, so the relative shader path resolves correctly.
4. Before running Editor, apply the [shader pass-selector repair](documentation/opengl-assessment.md#recommended-repair). The current loader's fixed byte offset depends on line endings and fails for the LF shader source in this checkout. This repair is documented but has not been applied to the source.

The [beginner guide](documentation/beginner-guide.md#2-prepare-the-windows-project) includes the full setup and a quoted, project-relative shader-minification command for the pre-build event.

## Quick start with shader audio

Edit [src/shaders/fragment.frag](src/shaders/fragment.frag):

| Function | Purpose |
| --- | --- |
| `m1` | Main visual pass. |
| `m2` | Post-processing pass; reads the scene texture through `sb1`. |
| `m3` | Music synthesis pass. |

The host selects each function by changing the shader's `#define m1 main` directive before compilation. Preserve the pass names and selector while editing. `m4` is reserved for optional audio post-processing and is not implemented in the supplied shader.

Build Editor and press **F5**. After saving shader changes, use **Ctrl+S** to trigger reload, or restart the application. The editor polls the key combination; it does not automatically watch file changes. By default, reload also regenerates the music. `SHAUDIO_UPDATEONSAVE` in [Audio_Shaudio.h](src/Audio_Shaudio.h) controls that regeneration.

The default sample is **1920 × 1080**, with **145 seconds** of stereo music at **44,100 sample frames per second**. The visual time uniform `m` is a sample-frame count; divide it by `44100.0` to obtain seconds.

To change duration, edit `SONG_DURATION` in [shaudio.h](src/shaudio.h) and ensure:

```text
SHAUDIO_XRES * SHAUDIO_YRES >= SAMPLE_RATE * SONG_DURATION
```

These C++ settings require a rebuild. If you change `SHAUDIO_XRES`, also update the audio row width hard-coded in `m3`. Visual resolution is separate: keep `XRES`/`YRES` in [definitions.h](src/definitions.h) consistent with `res` in the shader.

When the effect is ready, rebuild **Snapshot** for a quick compressed version, then **Release** for the final compression pass.

## Build configurations

| Configuration | Behavior | Output |
| --- | --- | --- |
| `Editor` | Controls, diagnostics, raw-shader reload, and pre-build shader minification. | `out/Lev4k-debug.exe` |
| `EditorNoRecompile` | Same editor behavior, with the pre-build minification step skipped. C++ still compiles when needed. | `out/Lev4k-debug.exe` |
| `Snapshot` | Embedded shader and Crinkler's `INSTANT` compression mode; no compression report. | `out/Lev4k-snapshot.exe` |
| `Release` | Embedded shader and Crinkler's `SLOW` compression mode with more optimization attempts. | `out/Lev4k-release.exe` and `out.html` |

Both Editor configurations share `bin/debug/` and their output executable. Snapshot and Release share `bin/release/`, but have distinct executable names. Build configurations sequentially and use **Rebuild** when switching them. Save any artifact you need to retain before a clean/rebuild operation.

Editor is windowed; Snapshot and Release enable fullscreen in the current source. The default shader-audio production builds exit at the end of the music. Editor loops playback. **Escape** exits either mode.

## Editor controls

These controls apply to the default shader-audio configuration:

| Keys | Action |
| --- | --- |
| Escape | Exit. |
| Ctrl+S | Reload shaders; regenerate music when `SHAUDIO_UPDATEONSAVE` is 1. |
| Alt+Up | Play. |
| Alt+Down | Pause. |
| Alt+Space | Seek to the beginning, preserving the current play/pause state. |
| Alt+Left / Alt+Right | Seek backward/forward by one second per polling iteration. |
| Alt+Shift+Left / Right | Seek backward/forward in smaller, 0.1-second steps per polling iteration. |

Holding a key can repeat the action every rendered frame. Camera controls update editor state, but the sample shader uses a fixed camera and comments out its camera uniforms. Connect those inputs to the shader before expecting camera movement to affect the image.

## Optional audio and export code

`AUDIO_TYPE` in [main.cpp](src/main.cpp) selects the audio adapter; its default is `AUDIO_SHAUDIO`. Alternative adapters need their own build and runtime integration.

| Component | Files and current status |
| --- | --- |
| 4klang | Place an exported `4klang.inc` in `src/4klang/`. Run `make.bat` from that directory to assemble `4klang.obj`, then integrate the object into the chosen configuration and select `AUDIO_4KLANG`. The main project does not automatically assemble or link it. |
| Oidos | Place `music.xrns` in `src/Oidos/`. Run `build_music_from_xrns.bat` from that directory to generate `music.asm`, `oidos.obj`, and `random.obj`. Integrate the objects and review the adapter's playback/editor handling before selecting `AUDIO_OIDOS`. |
| WAV playback | `Audio_Wave.h` uses the DirectShow-based `Song` class. Its current `track` lifetime/scope needs repair before use. |
| No audio | `AUDIO_NONE` advances the timeline by `1/60` second per rendered frame. This provides fixed-step export timing, not a live wall-clock timer. |
| Reverb and realtime shader audio | Disabled by default in `Audio_Shaudio.h`. Reverb requires an `m4` shader and further integration; realtime audio and MIDI are experimental paths. |

Recording switches are in `main.cpp`. Use separate runs for audio and images:

- **Audio:** set `RECORD_SFX` to 1 and use Editor with shader audio to request `recording.wav` output during audio initialization.
- **Images:** set `RECORD_IMG` to 1 and set `RECORD_IMG_LENGTH` in seconds. Image recording selects `AUDIO_NONE` and runs a fixed-step timeline of 60 frames per second. Frames are written under `export/`.
- **Image format:** `EXPORT_PNG` in [export.cpp](src/export.cpp) selects PNG when 1 and JPEG when 0. `KEEP_EXISTING_FRAMES` defaults to 1, so existing frame filenames are skipped.

Optional synths, realtime audio, MIDI, reverb, and recording were not validated in the feasibility study. Refer to the [validation record](documentation/validation-record.md) before relying on them for a production.

## Executable size and validation

Measure the final executable against the competition's exact byte limit. A 4,096-byte inclusive limit, strictly less than 4,096 bytes, and 4,000 bytes are different requirements. Source length and runtime memory usage are separate from executable size.

After a successful Release build, PowerShell can report the file size:

```powershell
(Get-Item -LiteralPath .\out\Lev4k-release.exe).Length
```

The documented Windows 11 experiments produced a **1,614-byte Snapshot** and a **1,573-byte Release**, using SDK retargeting and default structure alignment in a temporary copy. Shader compilation/linking was tested on one NVIDIA driver; complete audiovisual execution and native Ubuntu execution remain unvalidated. See the [recorded environment and results](documentation/validation-record.md).

## Productions and background

Productions listed by the original project include [Primordial Awakening (Revision 2023)](https://demozoo.org/productions/322371/) and [Drifting Shore (Revision 2024)](https://demozoo.org/productions/342196/). Production-specific effects and alternate variants are outside this `main`-branch guide.

## Acknowledgements

- [Leviathan 2.0](https://github.com/armak/Leviathan-2.0)
- [Shader Minifier](https://github.com/laurentlb/Shader_Minifier)
- [Crinkler](https://github.com/runestubbe/Crinkler)
- [4klang](https://github.com/hzdgopher/4klang)
- [dr_wav](https://github.com/mackron/dr_libs/blob/master/dr_wav.h)
- [stb_image_write](https://github.com/nothings/stb/blob/master/stb_image_write.h)

See [LICENSE](LICENSE) and the bundled dependencies' notices for licensing information.
