# Lev4K: feasibility study and student documentation

Assessment date: **19 September 2026**. The original assessment examined upstream `main` at `1e3d1654f495d82b91063dd8d23da34135d6d431`. The current checkout also includes the [LastFrameBuffer integration](last-frame-buffer.md).

**Lev4K already renders with OpenGL.** This checkout does not need a Direct3D-to-OpenGL conversion. Build settings and Editor shader reload have been updated; a native Ubuntu port still needs a platform layer. An optional audio-file component uses DirectShow; that is separate from the renderer.

| Requested topic | Finding | Read next |
| --- | --- | --- |
| Windows 11 | Feasible on Windows 11 x64. All four configurations build with the updated SDK selection and default structure alignment. Full audiovisual execution still needs acceptance testing. | [Feasibility study](feasibility-study.md) |
| OpenGL adaptation | OpenGL is already implemented. Retain the compatibility renderer initially; consider a core-profile renderer for portability. | [OpenGL assessment](opengl-assessment.md) |
| Linux / Ubuntu | A native functional port is feasible, but is not present. A sub-4K Linux executable needs a separate size experiment and release pipeline. | [Ubuntu porting study](ubuntu-porting-study.md) |
| Beginner C++ documentation | Guided Windows setup, code explanations, a first visual exercise, controls, and size measurement. Ubuntu setup is explicitly preparation for a future port. | [Beginner guide](beginner-guide.md) |
| Evidence and limitations | Local build results, shader probe, environment, and remaining validation gates. | [Validation record](validation-record.md) |
| LastFrameBuffer integration | Previous-frame feedback, reset behavior, student exercises, and current build/GPU results. | [Feedback guide](last-frame-buffer.md) |
| Upstream examples | Four local source examples; Drifting Shore can be prepared and built in an independent working directory. | [Examples guide](examples.md) |
| Other upstream branches | Seven branches reviewed for feedback, still-image accumulation, GPU experiments, production examples, and alternative music workflows. | [Branch review](branch-review.md) |

The repository includes the studies, English student documentation, and the LastFrameBuffer implementation. Build experiments ran in temporary copies; the repeatable GPU probe uses a hidden window.

## What has actually been demonstrated?

- `Editor` and `EditorNoRecompile` compile with the current project settings.
- `Snapshot` with feedback produces **1,770 bytes**.
- `Release` with feedback produces **1,724 bytes**, an increase of 151 bytes over the original sample.
- The generated visual, post-processing, and audio shaders compile and link on the tested NVIDIA OpenGL driver.
- Raw LF/CRLF selection, feedback accumulation, post-processing, history reset, and failed-reload recovery pass the GPU probe with mipmaps disabled and enabled.

These are build and automated GPU results, **not a claim that the complete intro has been watched and heard working**. No native Ubuntu build or execution was performed.

## Reading order

Students should read the [beginner guide](beginner-guide.md), followed by the [feedback exercise](last-frame-buffer.md). Instructors should run an interactive acceptance session on the classroom machines. Maintainers should read the [feasibility study](feasibility-study.md), then the OpenGL and Ubuntu studies. The validation record separates observed results from proposed work.

## Meaning of “4K” in this study

Use the competition's exact byte limit. The usual technical target discussed here is a **4,096-byte file limit**, while “4 kilobytes” can also mean 4,000 bytes. If the requirement is strictly *less than* 4,096 bytes, the maximum is 4,095 bytes. The measured Windows sample fits all three interpretations.

Measure the final submitted executable or complete counted payload, not source length, RAM usage, or the size of a ZIP containing it. Shared libraries and launcher scripts must satisfy the target competition's rules; this study does not assume that every external dependency is permitted.
