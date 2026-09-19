# Working with the upstream examples

The [local examples catalogue](../examples/README.md) contains Drifting Shore, Primordial Awakening, executable graphics, and Bonzo's image-atomic experiment. Original source excerpts and licenses are stored under each example's `upstream/` directory. Full commit IDs and file hashes are recorded in [manifest.json](../examples/manifest.json).

**The default LastFrameBuffer sample remains independent.** Its `src/`, Visual Studio project, generated shader, and build outputs are not replaced by example preparation. The three examples requiring different rendering architectures are available as source references. Drifting Shore has a prepared-copy workflow using the maintained host.

## Prepare Drifting Shore

From the repository root in PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\examples\prepare-example.ps1 -Name drifting-shore -Build Editor
```

The script prints the new solution path under `examples/_work/drifting-shore-<id>/`. Open that solution, select **Editor / x86**, and press **F5**. Edit the copy's `src/shaders/fragment.frag` and use **Ctrl+S** to reload. Its audio lasts 148 seconds and its default visual resolution is 1920 × 1080.

The command requires the repository's bundled Shader Minifier and Visual Studio 2022 C++ tools. MSBuild is found on PATH or through Visual Studio Installer's `vswhere`. To prepare without compiling C++, omit `-Build Editor`. Minification still runs inside the new copy.

To choose a destination, supply a new path:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\examples\prepare-example.ps1 -Name drifting-shore -Destination 'C:\work\Drifting Shore'
```

An existing destination is rejected, including an empty directory. Within this checkout, destinations must be below `examples/_work/`; other destinations must be outside the checkout. No directory is deleted or reused. If preparation fails, its partial copy remains available for inspection. Once prepared, the copy is independent and requires no Git branch switch or network access.

## Build a production version

In the prepared directory, from a Visual Studio Developer PowerShell:

```powershell
msbuild .\Lev4k.sln /t:Rebuild /p:Configuration=Release /p:Platform=x86
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
(Get-Item -LiteralPath .\out\Lev4k-release.exe).Length
```

Alternatively, prepare another copy with `-Build Release`. Builds never launch the intro automatically. Compression's progress GUI is disabled only in the prepared project. Each copy has its own intermediate and output directories, so experiments do not overwrite the default sample's binaries.

The prepared host differs from the original competition host. The measured adaptation is **4,229 bytes**, above a 4,096-byte limit; size optimization is a separate task. The main LastFrameBuffer sample retains its previous 1,724-byte measurement.

## Settings carried with the shader

Drifting Shore needs more than a fragment-file replacement:

| Setting | Prepared value and reason |
| --- | --- |
| Visual shader | Exact upstream source, including its comments and credits. |
| `USE_MIPMAPS` | `1`; the post-process samples mipmaps for bloom. |
| Audio configuration | Original `shaudio.h`: 148 seconds, 44,100 Hz, 1920 × 4000 render storage. |
| `RECORD_IMG_LENGTH` | `148`, if recording is later enabled. |
| Visual texture format | `GL_RGBA8`; the shader packs brightness into alpha and expects the original clamped representation. |
| Host/reload | Current maintained host, with separate visual/audio resources and validated Editor reload. |

The maintained host alternates two visual targets, but this shader does not read the previous main image. It therefore displays Drifting Shore without adding the sample's trails. The source reference's original `main.cpp` is provided for study and is not copied over the maintained host. `example-origin.json` in each prepared directory records the recipe and the source host's hash.

## Validation

Checked on **19 September 2026**, using the Windows 11 / Visual Studio 2022 / MSVC 14.41 / SDK 10.0.22621.0 environment in the [validation record](validation-record.md):

- Prepared Drifting Shore in a new directory whose name contains spaces; Editor and Release builds passed.
- Release file length: **4,229 bytes**; no sub-4K claim for this adaptation.
- A hidden-window GPU probe linked visual, post-processing, and music passes from raw LF, raw CRLF, and minified source on NVIDIA RTX 4060 / OpenGL 4.6.
- The minified main and post-processing passes rendered a nonblack 1920 × 1080 frame at a timeline position of 32 seconds, with RGBA8 targets and mipmaps.
- A small music-shader render produced finite values. This does not validate full music generation or audible playback.
- Reference files were checked against their pinned Git objects and manifest hashes. Main-project source, project settings, and generated shader remained unchanged.

Complete audiovisual playback, the exact compressed executable's startup, other GPU drivers, and Linux remain untested for the example. The other three source references were not compiled or executed during this import. Their renderer requirements are documented in their individual READMEs and the [branch review](branch-review.md).

## Suggested study order

1. Keep the default [LastFrameBuffer exercise](last-frame-buffer.md) as the first rendering lesson.
2. Open Drifting Shore's shader and study one feature at a time: camera table, scene intersections, color encoding, bloom, then music.
3. Use executable graphics to study sample accumulation before implementing its dedicated host.
4. Study Primordial Awakening's simulation and vertex stage, then Bonzo's image writes and synchronization.

Edit prepared working copies. The stored upstream excerpts serve as comparison points; their hashes are checked before preparation so accidental changes are detected.
