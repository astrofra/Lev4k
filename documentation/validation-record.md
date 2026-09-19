# Validation record and remaining acceptance tests

Assessment date: **19 September 2026**. Repository baseline: `main`, commit `1e3d1654f495d82b91063dd8d23da34135d6d431` (12 April 2025).

This page preserves the **original baseline experiments**. Current implementation and results are in the [LastFrameBuffer validation](last-frame-buffer.md#measurements-and-verification): Release is now 1,724 bytes, and the repeatable GPU probe checks rendering and reload recovery.

## Scope of the original experiments

Application sources, project files, and bundled tools in the repository were left unchanged. Builds ran in a temporary copy containing the root project/tool files and `src/`. The temporary project was adjusted progressively as recorded below. That initial delivery contained only Markdown documentation; source changes were introduced during the subsequent LastFrameBuffer integration.

No complete Lev4K application session was run. A small independent x86 probe created a **hidden Win32 window and legacy WGL context**, then compiled/linked the repository's shaders. It did not play sound, render the final intro, switch display modes, or verify the compressed executable's startup.

## Test environment

| Item | Observed value |
| --- | --- |
| OS | Microsoft Windows 11 Enterprise, version `10.0.26200`, 64-bit |
| Visual Studio | Community 2022, `17.11.4` |
| MSVC tools | `14.41.34120`, project toolset `v143` |
| Tested SDK | `10.0.22621.0` |
| SDK originally requested | `10.0.17763.0`, not installed |
| Build architecture | Win32 project / x86 solution |
| Bundled shader tool | Shader Minifier `1.3.5` |
| Bundled compression tool | Crinkler `2.1a`, 19 January 2019 |
| Probe GL vendor | `NVIDIA Corporation` |
| Probe GL renderer | `NVIDIA GeForce RTX 4060/PCIe/SSE2` |
| Probe GL version | `4.6.0 NVIDIA 591.86` |
| Probe GLSL version | `4.60 NVIDIA` |
| Other enumerated adapters | Intel UHD Graphics 630 and Parsec Virtual Display Adapter; neither tested by the probe |
| Linux | No native Linux test environment used; `wsl --list --quiet` listed no installed distribution |

The GPU strings describe the probe's actual context. Enumerating an adapter does not demonstrate rendering on it.

## Build experiments

| ID | Configuration and changes | Result |
| --- | --- | --- |
| B1 | Original `EditorNoRecompile`, original SDK selection | Failed: `MSB8036`, missing SDK `10.0.17763.0`. |
| B2 | `EditorNoRecompile`, command-line SDK override to `10.0.22621.0` | Built successfully. Crinkler forwarded to the MSVC linker. |
| B3 | `Editor`, same SDK override, original minifier command | Rebuilt successfully; minifier reported 1,139 characters of shader text. |
| B4 | `Snapshot`, SDK override; remove `/PROGRESSGUI` for unattended execution | Failed: `C2338`, Windows headers reject the project's 1-byte structure packing. |
| B5 | `Snapshot`, B4 settings plus `StructMemberAlignment=Default` | Rebuilt successfully; Crinkler reported **1,614 bytes**. |
| B6 | `Release`, B5 settings | Rebuilt successfully; final file measured **1,573 bytes**. |

The Editor executable measured 442,368 bytes. This is expected to be much larger than a compressed production build and has no 4K acceptance target.

Warnings included floating-point/integer conversions in existing code, an ignored `/QIfist` option in the compressed configurations, and an MSBuild warning about intermediate/output directories under the temporary directory. Crinkler also reported that its call transformation had no calls to transform in this sample. Successful rows are not claims of warning-free builds.

The Release rebuild removed the earlier Snapshot artifact during build cleanup. The 1,614-byte Snapshot result is preserved in its compression log; the Release size was independently checked through the resulting file's `Length`.

### Reproduction recipe

Use a separate working copy and the recorded compiler/tool versions for comparison. From a VS 2022 Developer PowerShell, these commands demonstrate the SDK selection:

```powershell
# B1: expected to fail only if the original SDK is absent.
msbuild .\Lev4k.sln /t:Build /p:Configuration=EditorNoRecompile /p:Platform=x86

# B2 and B3: use an SDK that is actually installed.
msbuild .\Lev4k.sln /t:Build /p:Configuration=EditorNoRecompile /p:Platform=x86 /p:WindowsTargetPlatformVersion=10.0.22621.0
msbuild .\Lev4k.sln /t:Rebuild /p:Configuration=Editor /p:Platform=x86 /p:WindowsTargetPlatformVersion=10.0.22621.0
```

For B4–B6, remove `/PROGRESSGUI` from the copy's Snapshot/Release linker options. B4 retains the original packing. Then change both occurrences of:

```xml
<StructMemberAlignment>1Byte</StructMemberAlignment>
```

to:

```xml
<StructMemberAlignment>Default</StructMemberAlignment>
```

Run B5 and B6 sequentially:

```powershell
msbuild .\Lev4k.sln /t:Rebuild /p:Configuration=Snapshot /p:Platform=x86 /p:WindowsTargetPlatformVersion=10.0.22621.0
if ($LASTEXITCODE -ne 0) { throw 'Snapshot failed' }
(Get-Item -LiteralPath .\out\Lev4k-snapshot.exe).Length

msbuild .\Lev4k.sln /t:Rebuild /p:Configuration=Release /p:Platform=x86 /p:WindowsTargetPlatformVersion=10.0.22621.0
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
(Get-Item -LiteralPath .\out\Lev4k-release.exe).Length
```

No application source correction was needed for these **build-only** results. The baseline Editor still needed the shader-selection repair at that stage; it is included in the current implementation.

### Identifying hashes

These SHA-256 hashes identify the exact bundled tools and measured Release artifact. They are not promises that every rebuild is byte-identical.

| File | SHA-256 |
| --- | --- |
| Repository `link.exe` | `2DC847527A858A86D150B857787F59FE186CB7CFD4A759D5A3E4230848C61125` |
| Repository `shader_minifier.exe` | `6FE8DCE492BB3B25C1F13E910D72A26ED22BCE5765B0EED9922D2F1ED58A6681` |
| Temporary `out/Lev4k-release.exe` | `DD4849684EA0BA23070AFB511427DB6ADE00A943426189D77A4921DE07559411` |

## Shader probe results

The raw `fragment.frag` contained 105 LF characters and no CRLF pairs. Its prefix was:

```text
#version 330\n#define m1 main\n
```

Here `\n` represents a line-feed byte. The pass digit is at zero-based index 22; index 23 is a space. The probe loaded the raw source in binary mode, or used a writable copy of the generated shader string, then called `glCreateShaderProgramv` and queried `GL_LINK_STATUS` plus its log.

| Input/selector | m1 | m2 | m3 |
| --- | --- | --- | --- |
| Raw LF source, current editor's index 23 for alternate passes | Pass | Fail | Fail |
| Raw LF source, corrected index 22 | Pass | Pass | Pass |
| Generated/minified shader, index 22 | Pass | Pass | Pass |
| Raw LF source, marker selector shown in the OpenGL assessment | Pass | Pass | Pass |
| Raw CRLF variant, same marker selector | Pass | Pass | Pass |
| Beginner gradient, marker selector | Pass | Pass | Pass |
| Beginner gradient after bundled minification, index 22 | Pass | Pass | Pass |

The original failing programs reported `error C3001: no program defined`. The marker test searched for `#define m1 main`, advanced by `strlen("#define m")`, and changed that digit. Both LF and CRLF variants were tested. This validates the selector's behavior for those inputs; the snippet was not yet integrated during that baseline experiment. The current Editor uses a marker selector and validates all active passes.

The gradient test extracted the exact GLSL example from [beginner-guide.md](beginner-guide.md), replaced `m1` and `m2` in the temporary shader, retained the supplied `m3`, and tested raw and generated passes. Compilation/linking passed; the expected animated appearance remains a runtime acceptance item.

## Findings from baseline source inspection

These are code-level findings, not additional runtime tests:

- Graphics is OpenGL; DirectShow is present in the optional audio-file component.
- The WGL setup does not explicitly request a version/profile or check initialization failures.
- Hot reload patches a fixed byte index and does not preserve the old program set on every failure.
- Release modifies generated string-literal storage; a portable C++ implementation must remove this assumption.
- Visual and audio dimensions/sample rates occur in multiple files and must remain consistent.
- `AUDIO_NONE` advances by a fixed amount per rendered frame.
- The optional WAV adapter has a `track` scope problem; optional reverb requires an absent `m4` and additional integration work.
- The default shader does not consume the editor's camera controls.
- Windows-specific source, build tools, and executable output prevent direct native Ubuntu compilation.

## Acceptance status at the baseline assessment

| Area | Test and success criterion | Current status |
| --- | --- | --- |
| Windows interactive editor | Start prepared Editor, see both visual passes, hear both channels, reload and recover from an intentional shader error | Not performed |
| Windows compressed executable | Run the exact measured Release from a folder without source assets; verify startup, complete duration, final exit, and sound | Not performed |
| Display handling | Windowed/fullscreen, 100/125/150% DPI, Alt+Tab, Escape, close request, multiple monitors; restore desktop state | Not performed |
| Graphics correctness | Reference images at fixed audio-frame positions; no missing or swapped passes | Shader linking only |
| GPU coverage | Repeat on NVIDIA, Intel, and AMD drivers where available; record actual GL renderer/profile | One NVIDIA shader probe only |
| Audio | Verify format, left/right order, duration, pause/seek, end-of-buffer handling, device errors, and drift | Not performed |
| Resource lifetime | Repeated reload and exit without growing program/texture/audio allocations | Not performed |
| Performance | Measure startup/music-generation time, frame rate at selected resolution, and RAM/VRAM use | Not performed; memory figures in the study are allocation arithmetic |
| Ubuntu build | Clean native build using documented package/tool versions | No port implemented |
| Ubuntu desktop | Test X11/XWayland and native Wayland separately, with real GPU and audio | Not performed |
| Ubuntu size | Count exact submitted payload, record dependencies, and run it on the agreed target | Not performed |
| Optional features | 4klang, Oidos, WAV, MIDI, reverb, realtime audio, PNG/WAV export | Not validated |

Suggested numerical targets for the implementation phase are a declared frame-rate goal (for example, 60 fps at 1080p on a named GPU) and a declared audio/video synchronization tolerance. Measure these on the intended machines; this study provides no performance guarantee.

Acceptance requires both correctness and the exact size limit. A tiny file that does not run, or an editor that works while the final artifact fails, does not pass.
