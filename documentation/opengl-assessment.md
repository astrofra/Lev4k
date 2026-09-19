# OpenGL assessment and adaptation plan

## The renderer already uses OpenGL

There is no active Direct3D renderer or HLSL shader to translate in this checkout.

| Evidence | Meaning |
| --- | --- |
| [definitions.h](../src/definitions.h) includes `<GL/gl.h>` and defines an OpenGL pixel format | The drawing surface is configured for OpenGL. |
| [main.cpp](../src/main.cpp) calls `wglCreateContext` and `wglMakeCurrent` | Windows creates and activates an OpenGL context. |
| [gldefs.h](../src/gldefs.h) resolves functions through `wglGetProcAddress` | Modern GL entry points come from the active graphics driver. |
| [fragment.frag](../src/shaders/fragment.frag) begins with `#version 330` | Effects are written in GLSL. |
| [Lev4k.vcxproj](../Lev4k.vcxproj) links `opengl32.lib` | The application links the Windows OpenGL interface. |

There are two qualifications to a repository-wide “no DirectX” claim. The optional `Song` audio-file class uses **DirectShow**, via `<dshow.h>` and COM. Oidos also contains an `ID3D11Texture2D_ID` symbol used as reusable data; its name does not indicate a Direct3D graphics path. Removing all DirectX-family dependencies would additionally require replacing or excluding the DirectShow audio-file component.

## Current rendering sequence

```mermaid
flowchart TD
    A[Create Win32 window and WGL context] --> B[Build m1, m2 and m3 programs]
    B --> C[m3 renders music into a float texture]
    C --> D[Read samples to RAM and start waveOut playback]
    D --> E[Read audio position and update uniform m]
    E --> F[m1 draws the scene into the window back buffer]
    F --> G[Copy scene pixels into a texture]
    G --> H[m2 reads that texture and draws the final image]
    H --> I[SwapBuffers]
    I --> E
```

This diagram describes the default shader-audio configuration. Music is generated at startup and, by default, again after editor shader reload. The scene uses the default framebuffer followed by `glCopyTexImage2D`; it is not initially rendered into a dedicated scene FBO.

## Existing compatibility assumptions

The runtime uses `glRects`, fragment-only programs, and the legacy `GL_LUMINANCE_ALPHA` transfer format. These depend on compatibility functionality. A switch to a core-profile context alone would break the renderer and music path.

`#version 330` describes the shader language version, not every host API requirement. `glCreateShaderProgramv` is available in OpenGL 4.1 or through `GL_ARB_separate_shader_objects`; the current code does not test either condition. An initial supported target should therefore explicitly require compatible GL functionality plus that API. Unused macros such as `glGenerateTextureMipmap` in `gldefs.h` do not by themselves impose an OpenGL 4.5 requirement. [Khronos OpenGL 4.1 reference card](https://www.khronos.org/files/opengl41-quick-reference-card.pdf), [separate shader objects specification](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_separate_shader_objects.txt).

On Windows, function addresses must be obtained for an appropriate current context. Add version/extension checks as well as pointer validation. [Microsoft `wglGetProcAddress` documentation](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-wglgetprocaddress).

## Confirmed shader-selection defect

The host compiles the same GLSL source repeatedly, changing this definition to select a different entry function:

```glsl
#version 330
#define m1 main
```

`MULTI_MAIN_LOCATION` is 22. In the generated string, the digit `1` is at zero-based byte 22. However, `refreshShaders` in [debug.h](../src/debug.h) modifies byte **23**, using `MULTI_MAIN_LOCATION + 1`.

The actual source file in this checkout uses **LF** line endings: byte 22 is `1`, and byte 23 is the following space. Replacing byte 23 creates `#define m12main` or `#define m13main`, so the intended entry function is absent. A hidden-window GPU probe confirmed that the original editor selection fails to link passes 2 and 3; selecting byte 22 succeeds. The generated/minified shader passes also link successfully. See the [validation record](validation-record.md).

CRLF after the first line happens to shift the digit to byte 23. Changing editor line-ending settings can therefore change behavior. A UTF-8 BOM, a new comment, or a different version directive can break both numeric offsets.

### Recommended repair

In Editor, find and validate the entry-selection marker by content. Work on a writable source buffer. Replace old programs only after **all** new programs have compiled and linked; report individual pass errors and delete abandoned shader/program objects.

The following small modification is a proposed teaching fix, not a change already applied to the repository. In `refreshShaders`, insert this immediately after `if (!newSource) return;`, before the first `shaderDebug` call:

```cpp
char* passNumber = strstr(newSource, "#define m1 main");
if (!passNumber) {
    fprintf(stderr, "Missing shader pass selector\n");
    free(newSource);
    return;
}
passNumber += strlen("#define m");
```

Then replace each assignment to `newSource[MULTI_MAIN_LOCATION + 1]` (including the optional `m4` case) with `*passNumber = '2';`, `*passNumber = '3';`, or `*passNumber = '4';` respectively. This handles either LF or CRLF for the exact selector shown. Keep that selector's spelling unchanged. This snippet fixes selection only; transactional reload and link diagnostics still need implementation.

For a larger refactor, supply the version directive, pass definition, and shared body as separate shader source strings. Keep `#version` first. This removes byte-offset patching entirely. Preserve pass functions during minification: the current command protects `m1,m2,m3,m4`, and preserves external uniform names. [Shader Minifier usage](https://github.com/laurentlb/shader-minifier).

The compact path also modifies a string literal after removing `const` through a macro. That is not valid general-purpose C++ practice: removing a qualifier does not make a string literal writable. A normal ELF or PE build may place it in read-only storage. Use an actual writable character array/copy or separate source strings in the portable implementation. Remeasure any change in the tiny Windows target.

## Two viable graphics strategies

| Strategy | Benefits | Costs and limits |
| --- | --- | --- |
| Retain compatibility rendering | Smallest initial change; keeps the Windows size baseline close to the original | Requires compatibility support and careful testing; preserves legacy drawing and audio formats. |
| Add an OpenGL 3.3 core renderer | Explicit, portable pipeline suitable for teaching and Ubuntu | Adds vertex-stage and object setup; changes the audio texture path; byte cost must be measured. |

**Recommended sequence:** repair the existing path, establish reference output, then develop the core renderer for the portable application. Keep the Windows tiny target available while measuring the new path.

### Core-profile implementation work

1. **Context creation:** request the intended version/profile explicitly. On Windows, an initial context is needed to load the WGL context-creation extension; create the final context, make it current, and load its functions. SDL2 can handle this in the portable host. [Khronos WGL context specification](https://registry.khronos.org/OpenGL/extensions/ARB/WGL_ARB_create_context.txt).
2. **Geometry:** replace every `glRects` / `glRectf` call, including music and optional realtime/reverb paths, with a fullscreen triangle. Use a vertex shader and a bound VAO; vertices can be generated from `gl_VertexID`.
3. **Programs:** for a true OpenGL 3.3 baseline, use `glCreateShader`, compilation, attachment, and normal linking of vertex and fragment shaders. Do not retain an unchecked dependency on `glCreateShaderProgramv`.
4. **Music format:** replace legacy luminance/alpha transfers. A two-channel `GL_RG32F` target with `GL_RG` / `GL_FLOAT` readback is a candidate. Change `m3` from `vec4(0, 0, mus)` to an output that stores left/right in red/green, such as `vec4(mus, 0, 1)`. Update reverb sampling too. Compare stereo channels and amplitude against the reference.
5. **Texture ownership:** allocate separate named music and post-process textures. The current hard-coded post texture name `1` can reuse a generated music texture name; make ownership explicit before adding passes.
6. **Dimensions:** separate display size from audio render dimensions. Pass actual framebuffer dimensions to the portable visual shaders; pass or generate a consistent audio row width. Resizing a window must not alter sample ordering.
7. **Diagnostics and lifetime:** check FBO completeness, texture-size limits, link logs, and GL errors; delete resources on reload and shutdown.

An `RG32F` music texture would halve the nominal GPU music storage relative to `RGBA32F`, but that saving and the complete rendering behavior have not been tested here.

## Validation before adoption

Compare original and adapted output at fixed audio-frame positions, for example 0, 44,100, and 441,000. Check images, stereo order, sample count, duration, pause/seek, end of playback, and reload failure recovery. Test at least NVIDIA and a Mesa-backed Intel or AMD system. Track startup time, frame rate, memory, and final executable bytes separately. A successful shader link or an attractive screenshot alone is insufficient to validate the port.
