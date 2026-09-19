# Lev4K: feasibility study and student documentation

Assessment date: **19 September 2026**. Repository examined: branch `main`, commit `1e3d1654f495d82b91063dd8d23da34135d6d431`.

**Lev4K already renders with OpenGL.** This checkout does not need a Direct3D-to-OpenGL conversion. It needs build maintenance, more reliable shader handling, and a platform layer for a native Ubuntu port. An optional audio-file component uses DirectShow; that is separate from the renderer.

| Requested topic | Finding | Read next |
| --- | --- | --- |
| Windows 11 | Feasible on Windows 11 x64. Editor builds after SDK retargeting; compressed builds also need default structure alignment. Full audiovisual execution still needs acceptance testing. | [Feasibility study](feasibility-study.md) |
| OpenGL adaptation | OpenGL is already implemented. Retain the compatibility renderer initially; consider a core-profile renderer for portability. | [OpenGL assessment](opengl-assessment.md) |
| Linux / Ubuntu | A native functional port is feasible, but is not present. A sub-4K Linux executable needs a separate size experiment and release pipeline. | [Ubuntu porting study](ubuntu-porting-study.md) |
| Beginner C++ documentation | Guided Windows setup, code explanations, a first visual exercise, controls, and size measurement. Ubuntu setup is explicitly preparation for a future port. | [Beginner guide](beginner-guide.md) |
| Evidence and limitations | Local build results, shader probe, environment, and remaining validation gates. | [Validation record](validation-record.md) |

The work delivered here is documentation and a feasibility assessment. Repository application sources and project settings have not been changed. Build experiments and a small hidden-window OpenGL probe ran in a temporary copy.

## What has actually been demonstrated?

- `Editor` and `EditorNoRecompile` compile with the installed Windows SDK selected.
- `Snapshot` produces **1,614 bytes** after correcting structure alignment in the temporary project.
- `Release` produces **1,573 bytes** with the same correction.
- The generated visual, post-processing, and audio shaders compile and link on the tested NVIDIA OpenGL driver.
- The editor's existing shader pass selector fails for the LF line endings in this checkout. The probe reproduces the failure and confirms the corrected offset works.

These are build and shader results, **not a claim that the complete intro has been watched and heard working**. No native Ubuntu build or execution was performed.

## Reading order

Students should read the [beginner guide](beginner-guide.md), with an instructor preparing the known fixes before the first session. Maintainers should read the [feasibility study](feasibility-study.md), then the OpenGL and Ubuntu studies. The validation record separates observed results from proposed work.

## Meaning of “4K” in this study

Use the competition's exact byte limit. The usual technical target discussed here is a **4,096-byte file limit**, while “4 kilobytes” can also mean 4,000 bytes. If the requirement is strictly *less than* 4,096 bytes, the maximum is 4,095 bytes. The measured Windows sample fits all three interpretations.

Measure the final submitted executable or complete counted payload, not source length, RAM usage, or the size of a ZIP containing it. Shared libraries and launcher scripts must satisfy the target competition's rules; this study does not assume that every external dependency is permitted.
