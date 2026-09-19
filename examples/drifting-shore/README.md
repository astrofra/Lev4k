# Drifting Shore

By NuSan, for Revision 2024. Source: [TheNuSan/Lev4k, commit 60721c6605ebc664be2071e3915518e1d57e9014](https://github.com/TheNuSan/Lev4k/tree/60721c6605ebc664be2071e3915518e1d57e9014). See the included [MIT license](upstream/LICENSE).

Start with the commented [fragment shader](upstream/src/shaders/fragment.frag). It contains all three passes: visual rendering, post-processing, and music. The [original host](upstream/src/main.cpp), [audio settings](upstream/src/shaudio.h), and [GL bindings](upstream/src/gldefs.h) document its dependencies.

The example uses 18 scene/camera entries, analytic intersections, stochastic reflections, depth of field, bloom from mipmaps, and procedural music. Study the camera table and post-processing before tackling the complete shader.

Prepare a working copy from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\examples\prepare-example.ps1 -Name drifting-shore -Build Editor
```

The copy uses the maintained framework with these settings:

- `USE_MIPMAPS = 1` for bloom.
- Original `shaudio.h`: 148 seconds of music, 44,100 Hz, a 1920 × 4000 audio texture.
- `RECORD_IMG_LENGTH = 148`, matching the music duration if recording is later enabled.
- RGBA8 visual targets, preserving the shader's RGB/alpha brightness encoding and clamping.

The shader itself is unchanged. Its visual pass does not use temporal feedback, although the maintained host still alternates two visual targets. Editor reload and separate audio/visual resources remain available. No file in the main framework is replaced.

Editor and Release builds passed in the documented Windows environment. This host adaptation measured **4,229 bytes** in Release, **133 bytes above an inclusive 4,096-byte limit**. It is suitable as a working example; competition-size optimization is separate. GPU linking and a frame render passed, but the complete audiovisual performance and exact compressed executable have not been run. See the [validation details](../../documentation/examples.md#validation).
