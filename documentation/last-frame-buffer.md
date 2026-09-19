# LastFrameBuffer: temporal feedback

The current sample includes an adaptation of the upstream [LastFrameBuffer branch](https://github.com/TheNuSan/Lev4k/tree/ae3d69f89102e5896efe981ebc84a7ffe025e98c). It retains the existing raymarcher, color-channel post-processing, and shader-generated music, and adds trails by mixing each new image with the previous one.

## How it works

Two floating-point textures alternate roles. A framebuffer object (FBO) lets OpenGL draw into a texture. Reading one texture while writing the other avoids reading pixels that the same draw is still changing.

```text
Previous m1 image -> sb1 -> m1 -> current texture -> sb1 -> m2 -> window
                                |
                                +-> previous image on the next frame
```

In `m1`, `sb1` contains the previous **main pass**, before post-processing. In `m2`, the same sampler name contains the **current main pass**. The host binds the appropriate texture before each draw. The post-process never writes back into history, so its color offsets do not compound on every frame.

The sample's GLSL expression is:

```glsl
col = mix(col, texture(sb1, gl_FragCoord.xy/res.xy).rgb, 0.95);
```

For a history weight `w`, the new stored color is `(1 - w) * current + w * previous`. With a constant white input and initially black history, the first red-channel values are `0.05`, `0.0975`, and `0.142625`.

Try `0.5`, `0.8`, and `0.95` and compare the trails. Set the weight to `0.0` to display only the new image; the host still allocates the two textures. A weight of `1.0` would retain the initial black history indefinitely.

This weight is **per rendered frame**, matching the upstream example. Higher frame rates shorten its persistence in seconds. Music pause freezes scene time, while feedback continues to converge each frame. A later time-based variant would need an elapsed-time input and a decay formula.

## C++ implementation and resets

| File | Responsibility |
| --- | --- |
| [feedback.h](../src/feedback.h) | Allocate two textures and FBOs, configure filtering/wrapping, check FBO completeness in Editor, and clear history. |
| [main.cpp](../src/main.cpp) | Select read/write targets, draw both passes, alternate targets, and request Editor resets. |
| [fragment.frag](../src/shaders/fragment.frag) | Mix the new scene color with history. |
| [debug.h](../src/debug.h) | Load and link replacement shader programs; retain the existing set on failure. |

Both textures use `GL_RGBA32F`, linear filtering, and edge clamping. Their contents are explicitly cleared before the first frame. Visual textures and framebuffer names are generated separately from the music resources. The old `glCopyTexImage2D` scene copy has been removed.

For the default shader-audio Editor, history is cleared after:

- A successful shader reload, including any associated music regeneration.
- Seeking backward or forward, returning to the beginning, or looping at the end.
- A change between play and pause.

A failed shader reload retains both the existing programs and image history. A reset clears both textures and restarts their alternation. It intentionally rebuilds trails from black, rather than trying to reconstruct the sequence of images preceding the new timeline position.

Resolution is fixed by `XRES`/`YRES` and GLSL `res`. Change both and restart after rebuilding; dynamic resizing is not implemented. A future resize handler must reallocate both textures and clear their contents. `USE_MIPMAPS` remains disabled by default; when enabled, cleared textures and completed frames have their mipmaps generated before sampling.

## Build and reload maintenance included

The project selects an installed Windows 10 SDK through version `10.0`, uses default structure alignment for Snapshot/Release, quotes the shader tool paths, and supplies the project directory as Editor's debugging working directory.

Editor finds the `#define m1 main` marker by its content, so LF and CRLF line endings are accepted. It checks link status for every active pass and replaces the program set only when all passes succeed. Failed candidates are deleted, and successful reloads release the previous programs. Preserve the selector spelling and pass names.

The compact production path retains its existing generated-source offset of 22. Regenerate `fragment.inl` after editing the shader. The regression probe checks this offset against the actual generated shader and links all three production passes.

## Measurements and verification

Measured on **19 September 2026**, using the Windows 11 / MSVC 14.41 / SDK 10.0.22621.0 / NVIDIA RTX 4060 environment recorded in the [baseline validation record](validation-record.md). Builds used a temporary copy of the integrated sources. Only `/PROGRESSGUI` was removed from that copy's compression options for unattended operation.

| Check | Result |
| --- | --- |
| Editor / x86 | Build passed. |
| EditorNoRecompile / x86 | Build passed. |
| Snapshot / x86 | Build passed; Crinkler reported **1,770 bytes**. |
| Release / x86 | Build passed; file length **1,724 bytes**. |
| Change from the original 1,573-byte Release | **+151 bytes**. |
| Raw LF, raw CRLF, and minified shader passes | All three passes linked on the tested driver. |
| Actual sample rendering | Finite, nonblack output; resetting reproduced the same first image at a fixed time. |
| Deterministic feedback fixture | GPU readback matched the expected color recurrence. |
| Post-processing | Read the latest main image without changing stored history. |
| Failed reloads | Missing entry points in each pass, and a missing selector, preserved the active program set; history and audio-reset state were retained. |
| Valid reload after failure | Recovered and released the old programs. |
| Optional mipmaps | The same GPU probe passed with `USE_MIPMAPS=0` and `1`, without GL errors. |

Release SHA-256: `4B39DA4348A89872F45BF671E50D6279139D3C6E24B7A8B281F195C2F5581B93`.

The subsequent [CMake build](cmake-build.md) reproduces that Release byte-for-byte and exposes the GPU probes through CTest and `build_tests.bat`.

At 1920 × 1080, the two base-level textures occupy `2 * 1920 * 1080 * 4 * 4 = 66,355,200` bytes, approximately **63.3 MiB**. Optional mipmaps add storage. These figures exclude the music buffers, window buffers, and driver allocations.

The automated probe uses a hidden WGL window and real GPU rendering, without sound or display-mode changes. It exercises the shared allocation/reset and reload implementations, with a test rendering loop. Keyboard routing, a complete audible Editor session, fullscreen behavior, and execution of the exact compressed binary remain interactive acceptance checks. No Ubuntu or additional GPU-driver validation is claimed.

## Reproduce the GPU checks

From an **x86 VS 2022 Developer PowerShell**, run:

```powershell
.\tests\run-feedback-probe.ps1
```

The [script](../tests/run-feedback-probe.ps1) builds [feedback-probe.cpp](../tests/feedback-probe.cpp) and runs it twice, with mipmaps disabled and enabled. It writes its binaries under the temporary directory, leaving the application outputs alone. Expected invalid-shader diagnostics appear during the recovery tests; each run must end in `PASS` and return exit code zero. If local policy blocks scripts, use `powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\run-feedback-probe.ps1` from the same developer environment.

For an interactive check, build Editor, inspect the trails, reload a valid shader, introduce and repair a shader error, then try pause, resume, and both seek directions. Confirm that successful reloads and time changes restart the trails. Build Release and measure the exact artifact before testing it on the intended playback machine.
