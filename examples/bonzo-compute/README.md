# Bonzo image atomics

Source: [TheNuSan/Lev4k, commit 2246ddd5ee1ba6bd9ed57012f78b8b4dead346ea](https://github.com/TheNuSan/Lev4k/tree/2246ddd5ee1ba6bd9ed57012f78b8b4dead346ea). See the included [MIT license](upstream/LICENSE).

The [fragment shader](upstream/src/shaders/fragment.frag) uses image loads and atomic integer additions to draw lines and retain fading history. The [host](upstream/src/main.cpp) creates six integer images; [gldefs.h](upstream/src/gldefs.h) shows the additional GL functions. The [audio settings](upstream/src/shaudio.h) are also preserved.

Despite the branch name, it uses a fragment shader rather than a compute-shader dispatch. Its host requires image bindings, image clearing, and synchronization beyond the default two-texture feedback path.

Before making a runnable example, initialize both image sets, fix uniform binding assumptions, and add the required memory barriers. The use of GLSL 4.20 and `glClearTexImage` also needs an explicit graphics feature requirement. The current preparation script does not build this reference; the [branch review](../../documentation/branch-review.md#4-bonzo_compute-advanced-fragment-shader-writes) describes the details.
