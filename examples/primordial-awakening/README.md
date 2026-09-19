# Primordial Awakening

Source: [TheNuSan/Lev4k, commit 3fb8f8180785651c774ae754c3dfae3e5f0e71b1](https://github.com/TheNuSan/Lev4k/tree/3fb8f8180785651c774ae754c3dfae3e5f0e71b1). See the included [MIT license](upstream/LICENSE).

The imported source shows a 500,000-point draw with particle state held in textures:

- [Fragment shader](upstream/src/shaders/fragment.frag): particle appearance, simulation, post-processing, and music.
- [Vertex shader](upstream/src/shaders/vertex.vert): fetch particle state using `gl_VertexID`, position points, and set their size.
- [Host](upstream/src/main.cpp): rotate the simulation textures, configure program pipelines, and draw the particles.
- [Shader loader](upstream/src/debug.h), [audio adapter](upstream/src/Audio_Shaudio.h), [audio settings](upstream/src/shaudio.h), and [GL bindings](upstream/src/gldefs.h): associated setup.

This is a source reference with its own rendering architecture. Its `m1` draws particles, `m2` simulates, `m3` post-processes, and `m4` generates music. Replacing the default framework's fragment shader with it would select the wrong passes and omit the vertex stage and simulation resources.

A runnable adaptation needs a dedicated particle host, explicit texture/framebuffer ownership, and initialization/reset behavior for pause and seeking. The current preparation script therefore does not build this example. The [branch review](../../documentation/branch-review.md#5-primordialawakening-a-substantial-particle-example) records the required work. The original source is preserved for study without changing the main framework.
