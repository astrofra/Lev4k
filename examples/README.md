# Upstream examples

Four visual examples are available locally, with their original shaders, related host code, license, and pinned upstream commit. These files are independent of the default LastFrameBuffer sample.

| Example | What to study | Available workflow |
| --- | --- | --- |
| [Drifting Shore](drifting-shore/README.md) | Camera sequencing, reflections, depth of field, bloom, and shader music. | Prepare and build a separate working copy with the maintained host. |
| [Primordial Awakening](primordial-awakening/README.md) | Texture-based simulation and 500,000 particles. | Read the original fragment/vertex shaders and particle host; a dedicated host adaptation is required. |
| [Executable graphics](exe-gfx/README.md) | Accumulating multiple samples into a still image. | Read the shader, accumulation loop, and fixed-step clock; a dedicated host adaptation is required. |
| [Bonzo image atomics](bonzo-compute/README.md) | Drawing through integer images and atomic additions. | Read the shader, image setup, and GL bindings; a dedicated host adaptation is required. |

From the repository root, in PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\examples\prepare-example.ps1 -Name drifting-shore -Build Editor
```

The command creates a new directory under `examples/_work/`, prints its location, and builds its own solution. Open that solution and select **Editor / x86** to run it. Each invocation creates a new copy; the original project's source, settings, generated shader, and outputs are not written by the script.

Omit `-Build Editor` to prepare the solution for Visual Studio without compiling C++. Shader minification still runs, so the embedded shader is ready. Use `-Destination 'C:\work\Drifting Shore'` to choose a new directory outside the repository; an existing destination is rejected. The script does not start the intro automatically.

The `upstream/` directories contain **selected source excerpts**, not complete historical checkouts. Keep them as references and edit a prepared working copy. The script imports Drifting Shore's shader and audio settings; its original `main.cpp` is included for comparison, not overlaid onto the maintained host.

[manifest.json](manifest.json) records the source repository, full commit IDs, and SHA-256 hashes for every imported file. Each example includes its upstream MIT license and preserves the source comments and credits. Git attributes preserve the reference files' bytes across checkouts.

See the [examples guide](../documentation/examples.md) for build details, measurements, and the scope of validation. The [branch review](../documentation/branch-review.md) explains the architecture of each variant.
