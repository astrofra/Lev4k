# Review of the other upstream branches

Review date: **19 September 2026**. Repository: [TheNuSan/Lev4k](https://github.com/TheNuSan/Lev4k). Comparison baseline: upstream `main` at `1e3d1654f495d82b91063dd8d23da34135d6d431`.

**The most immediately useful additions are `LastFrameBuffer` for temporal effects and `DriftingShore` for visual and musical examples.** `exe_gfx` provides a useful still-image rendering mode. `bonzo_compute` and `PrimordialAwakening` offer more advanced GPU techniques, with additional integration work. The two synthesizer branches mainly demonstrate alternative audio configurations.

This is a source and Git-history review of all seven non-`main` branches advertised by the upstream repository on the review date. A separate temporary bare clone was used; the working checkout and its branch references were left unchanged. None of these branch versions was compiled or executed. Integration risks below are findings from source inspection, not observed runtime failures. The Windows build measurements in the [validation record](validation-record.md) apply only to the baseline sample.

**Implementation follow-up:** LastFrameBuffer has now been adapted into the working checkout, with initialization, reset handling, and validated shader reload. Its 1,724-byte Release and GPU checks are documented in the [feedback guide](last-frame-buffer.md). The branch snapshots reviewed below remain the upstream versions.

Source excerpts from Drifting Shore, Primordial Awakening, exe_gfx, and bonzo_compute are now [available locally](../examples/README.md). Drifting Shore also has an independent preparation/build workflow; see the [examples guide](examples.md).

## Branch inventory

Links identify the exact snapshots examined, so the review remains reproducible if branch heads move.

| Branch and reviewed commit | Head commit date | Main contribution | Suggested use |
| --- | --- | --- | --- |
| [`LastFrameBuffer` / `ae3d69f`](https://github.com/TheNuSan/Lev4k/tree/ae3d69f89102e5896efe981ebc84a7ffe025e98c) | 2023-07-31 | Two alternating floating-point render targets; previous-frame feedback. | First reusable rendering extension. |
| [`DriftingShore` / `60721c6`](https://github.com/TheNuSan/Lev4k/tree/60721c6605ebc664be2071e3915518e1d57e9014) | 2024-04-04 | A commented production shader with camera sequencing, reflections, depth of field, bloom, and music. | Extract examples and study a complete production. |
| [`exe_gfx` / `ea10254`](https://github.com/TheNuSan/Lev4k/tree/ea10254f33689034a44847e3353167622a4c407f) | 2025-04-09 | Accumulation of multiple samples into a final still image, with audio disabled. | Optional executable-graphics mode. |
| [`bonzo_compute` / `2246ddd`](https://github.com/TheNuSan/Lev4k/tree/2246ddd5ee1ba6bd9ed57012f78b8b4dead346ea) | 2025-04-12 | Integer image storage and atomic writes from a fragment shader. | Advanced experiments with drawing and simulation. |
| [`PrimordialAwakening` / `3fb8f81`](https://github.com/TheNuSan/Lev4k/tree/3fb8f8180785651c774ae754c3dfae3e5f0e71b1) | 2023-04-21 | Texture-based particle simulation and a 500,000-point draw. | Advanced particle-system case study. |
| [`4klang` / `0c59058`](https://github.com/TheNuSan/Lev4k/tree/0c59058db47f7f81be14edda6658fd104ad31b85) | 2024-03-27 | Selection and linking of the 4klang audio backend. | Optional workflow for music composed outside GLSL. |
| [`oidos` / `245f667`](https://github.com/TheNuSan/Lev4k/tree/245f6672dff20862cd47c834a2551ae1ee47b430) | 2024-03-27 | Oidos audio selection, linking, and calling-convention changes. | Optional Renoise-based workflow after integration repairs. |

## 1. LastFrameBuffer: the best first extension

The host creates two `GL_RGBA32F` textures attached to framebuffer objects. A framebuffer object, or FBO, lets OpenGL draw into a texture instead of directly into the window. Each frame reads the previous texture through `sb1` and writes into the other texture, then displays the new result. This alternating arrangement is commonly called *ping-pong rendering*.

The sample mixes the current color with the previous image using a history weight of `0.95`. This directly provides trails and temporal feedback. The same infrastructure could support other effects, such as reaction-diffusion simulation, after writing the corresponding shader. Rendering directly into an FBO also replaces the baseline scene-copy operation.

Before integrating it:

- Allocate, configure, and clear both textures before their first use. The branch allocates storage with a null data pointer and does not explicitly initialize the history.
- Reset history after a shader reload, timeline jump, or resolution change.
- Set texture filtering before sampling and check framebuffer completeness in Editor.
- Decide whether decay should depend on elapsed time. A constant weight per frame changes the appearance when the frame rate changes.

At 1920 × 1080, the two RGBA32F textures require approximately **63.3 MiB** of texture storage. That is GPU memory, not executable size. This is a compact and understandable extension for teaching persistent visual state, after students have learned the baseline shader passes. See the reviewed [host implementation](https://github.com/TheNuSan/Lev4k/blob/ae3d69f89102e5896efe981ebc84a7ffe025e98c/src/main.cpp) and [shader](https://github.com/TheNuSan/Lev4k/blob/ae3d69f89102e5896efe981ebc84a7ffe025e98c/src/shaders/fragment.frag).

## 2. DriftingShore: the richest source of examples

The approximately 690-line [fragment shader](https://github.com/TheNuSan/Lev4k/blob/60721c6605ebc664be2071e3915518e1d57e9014/src/shaders/fragment.frag) is extensively commented and keeps the familiar `m1` visual, `m2` post-processing, and `m3` music structure.

Useful material includes:

- An 18-entry scene and camera table, selected in eight-second intervals, with seeds, camera paths, field of view, and focus parameters.
- Analytic intersections for spheres, planes, and carved boxes, plus stochastic reflections and depth of field.
- A color encoding that stores a brightness multiplier in alpha alongside RGB, then reconstructs brighter values during post-processing.
- Mipmap-based bloom, vignette, tone mapping, and fades.
- Procedural music using detuned partials, envelopes, seeded timbres, and time-dependent arrangement of melodic, bass, pad, and percussion parts.

These are good examples to extract into smaller lessons. Start with the camera table or tone mapping; introduce the full renderer and synthesizer later. The color encoding and its decoder must stay together, and the bloom requires mipmap generation. The branch also changes the audio duration and buffer capacity; copying only the shader would miss those associated settings.

The renderer uses 25 samples per pixel and up to three ray segments per sample. Its performance needs measurement at the intended resolution. The RGB-plus-alpha encoding uses an RGBA8 target and has a bounded range; it should not be presented to students as a floating-point HDR buffer.

## 3. exe_gfx: accumulating a still image

This branch selects `AUDIO_NONE` and accumulates successive rendered samples in one RGBA32F texture. Additive blending sums color into RGB and sample count into alpha. The presentation shader divides the color sum by the count to obtain an average. Editor can show the image developing; the production configuration waits until accumulation is finished before displaying it. See the [host](https://github.com/TheNuSan/Lev4k/blob/ea10254f33689034a44847e3353167622a4c407f/src/main.cpp) and [shader](https://github.com/TheNuSan/Lev4k/blob/ea10254f33689034a44847e3353167622a4c407f/src/shaders/fragment.frag).

This is useful infrastructure for executable graphics and could be adapted for stochastic antialiasing or depth of field. The supplied sample varies scene time during accumulation; it is not a complete path tracer.

`EXE_GFX_DURATION = 3` is measured using the no-audio clock, which advances by `1/60` on each update. It therefore represents roughly 180 iterations, not a guarantee of three seconds of wall-clock rendering time. An explicit sample-count setting would be clearer for a student-facing implementation.

Two host details need repair: texture and framebuffer identifiers are generated into opposite variables before binding, and the accumulation buffer is not explicitly cleared before additive accumulation. Generate each object into the correct variable, clear its contents, guard against a zero sample count, and reset accumulation when scene parameters change. One 1080p RGBA32F texture uses approximately **31.6 MiB**.

## 4. bonzo_compute: advanced fragment-shader writes

Despite the branch name, this implementation uses a **fragment shader**, without `glDispatchCompute` or a compute-shader stage. Six `R32UI` images hold two sets of three color channels. Shader invocations can write to arbitrary image coordinates with `imageAtomicAdd`, while reading the previous set with `imageLoad`. Colors are scaled to integers for storage. The sample uses this to draw lines and retain fading history. See the [shader](https://github.com/TheNuSan/Lev4k/blob/2246ddd5ee1ba6bd9ed57012f78b8b4dead346ea/src/shaders/fragment.frag) and [host](https://github.com/TheNuSan/Lev4k/blob/2246ddd5ee1ba6bd9ed57012f78b8b4dead346ea/src/main.cpp).

This permits a different style of effect: an invocation can contribute to a pixel elsewhere in the image, rather than only producing its own fragment's color. Particle splats and procedural line drawing are plausible extensions. Atomic addition makes concurrent additions to the same stored value indivisible; it does not remove the need to synchronize rendering passes.

The branch uses GLSL 4.20 and `glClearTexImage`. The latter is core in OpenGL 4.4, with an extension route through `ARB_clear_texture`. An unmodified adoption therefore does not fit the proposed OpenGL 3.3 core baseline for Ubuntu. It also retains legacy drawing calls, so selecting a modern core context alone is insufficient. See the [Khronos versioned API header](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/api/GL/glcorearb.h) and [clear-texture extension](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_clear_texture.txt).

Before reuse:

- Add the appropriate memory barriers between image writes, later reads, and texture clears. No `glMemoryBarrier` call appears in this branch's implementation. `coherent` does not by itself establish all required ordering between rendering commands; the [image-load/store specification](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_shader_image_load_store.txt) describes the barrier requirements.
- Set image uniform values using queried locations, or use supported explicit bindings. The current `glUniform1i(i, i)` assumes locations happen to be consecutive integers matching image units.
- Initialize both image sets before the first frame.
- Define the acceptable integer range and behavior for negative or overflowing contributions when extending the algorithm.

The six 1080p R32UI images use approximately **47.5 MiB**. This is the most current experimental branch: it contains the reviewed `main` commit and changes only four files relative to it. Keep it as an optional advanced renderer until synchronization, portability, performance, and compressed size have been tested.

## 5. PrimordialAwakening: a substantial particle example

The host issues `glDrawArrays(GL_POINTS, 0, 500000)`. Particle state is stored in floating-point textures, and the vertex shader looks up state using `gl_VertexID`. The simulation derives motion from previous positions and applies procedural forces around distance-field geometry. Point rendering and post-processing produce the final image. See the [host](https://github.com/TheNuSan/Lev4k/blob/3fb8f8180785651c774ae754c3dfae3e5f0e71b1/src/main.cpp), [vertex shader](https://github.com/TheNuSan/Lev4k/blob/3fb8f8180785651c774ae754c3dfae3e5f0e71b1/src/shaders/vertex.vert), and [fragment shader](https://github.com/TheNuSan/Lev4k/blob/3fb8f8180785651c774ae754c3dfae3e5f0e71b1/src/shaders/fragment.frag).

This branch is an **ancestor of `main`**, rather than an unmerged extension. The subsequent history simplified the production into the current framework. Merging this branch into `main` would not restore its particle system; the relevant implementation needs to be extracted and adapted.

Integration is substantial. There are three rotating RGBA32F textures, a custom vertex stage, and different pass roles: `m1` draws particles, `m2` simulates them, `m3` post-processes, and `m4` generates music. The shaders also use legacy built-ins that need replacement for a core-profile port.

Repair object ownership where framebuffer names are used as texture names. Handle initialization and discontinuities in simulation time: deriving velocity by dividing by a time difference needs defined behavior for pause, backward seeking, and restarting. Three 1080p RGBA32F textures alone use approximately **94.9 MiB**. This is suitable for an advanced GPU programming exercise once the simpler feedback example is understood.

## 6. 4klang and oidos: alternative music workflows

The baseline already contains source and adapters for these optional synthesizers. Their branches mainly supply selection and build wiring, rather than new rendering capabilities.

**`4klang`** selects `AUDIO_4KLANG` and adds the generated `4klang.obj` to linker inputs. In that snapshot, the generation script is named `src/4klang/make_music.bat`. The object must be generated; it is not supplied as a ready-made checked-in object. This is worth retaining as an optional lesson for musicians who prefer a tracker or DAW workflow to writing a GLSL synthesizer. Review the [project settings](https://github.com/TheNuSan/Lev4k/blob/0c59058db47f7f81be14edda6658fd104ad31b85/Lev4k.vcxproj) and regenerate the music before measuring its effect on the final executable.

**`oidos`** selects the Oidos backend and links its generated music and random-data objects. It is relevant to a Renoise workflow, but its current configuration needs more repair. The branch removes the assembly's decorated `__stdcall` aliases while changing the C++ calling convention to `__cdecl` only for Editor; the other configurations retain `__stdcall`. The project also leaves a 4klang object dependency in EditorNoRecompile. These settings should be reconciled across configurations before treating the branch as a ready build preset. See the [project settings](https://github.com/TheNuSan/Lev4k/blob/245f6672dff20862cd47c834a2551ae1ee47b430/Lev4k.vcxproj).

Oidos also has its own playback-position mechanism. The generic editor controls and end-of-playback condition still need review against that mechanism; selecting the backend alone does not establish that pause, seek, and automatic exit work correctly.

For Ubuntu, both options need an appropriate native object format, matching architecture and calling convention, and adapted audio playback. A Windows x86 object cannot simply be linked into a Linux executable. Shader audio remains the simplest starting point for the port because it follows the baseline path already under study.

## Integration priorities for this project

1. Apply the baseline Windows and shader-loading repairs described in the [OpenGL assessment](opengl-assessment.md), then establish a functional native platform layer as described in the [Ubuntu study](ubuntu-porting-study.md). No reviewed branch supplies a native Linux implementation or a Linux build system.
2. Extract `LastFrameBuffer` into a small optional example with explicit initialization and reset behavior. It offers the clearest next teaching step after the existing two-pass sample.
3. Use `DriftingShore` to build individual lessons on cameras, composition, lighting, post-processing, and music. Preserve its coupled shader and host settings when reproducing the full production.
4. Add `exe_gfx` if still-image productions are part of the curriculum. Keep accumulation and sample-count controls understandable.
5. Reserve `bonzo_compute` and `PrimordialAwakening` for advanced GPU work. Choose the required OpenGL feature level explicitly before incorporating either into the Ubuntu renderer.
6. Add a synthesizer workflow when a production or musician needs it. Start with `4klang`; repair all Oidos configurations before offering them to students.

Use selected changes against the maintained baseline. Older snapshots lack later fixes and contain older project settings, so replacing complete current files with branch versions would lose maintenance work. `exe_gfx` is close to current `main`; `bonzo_compute` already includes it. `PrimordialAwakening` requires extraction from historical code.

For every adopted feature, validate shader loading, first-frame contents, reload and seeking behavior, visual output, audio where applicable, and compressed Release size. Several snapshots contain a tracked debug executable, but no final production binary was found to establish their current 4K size. The original branch snapshots were not size-validated in this review. The subsequent LastFrameBuffer adaptation fits below 4,096 bytes; other combinations still require measurement. GPU memory estimates above are calculated texture payloads at 1920 × 1080; they exclude audio buffers and driver allocations.
