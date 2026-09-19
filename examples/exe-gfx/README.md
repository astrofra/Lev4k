# Executable graphics

Source: [TheNuSan/Lev4k, commit ea10254f33689034a44847e3353167622a4c407f](https://github.com/TheNuSan/Lev4k/tree/ea10254f33689034a44847e3353167622a4c407f). See the included [MIT license](upstream/LICENSE).

The [host](upstream/src/main.cpp) accumulates multiple rendered samples using additive blending. The [fragment shader](upstream/src/shaders/fragment.frag) stores a color sum and sample count, then averages them for presentation. The [no-audio clock](upstream/src/Audio_None.h) advances by a fixed `1/60` per iteration. The [original shader loader](upstream/src/debug.h), [GL bindings](upstream/src/gldefs.h), and [audio settings](upstream/src/shaudio.h) are included as context.

This is useful for studying executable still images and supersampling. Its duration setting represents an iteration budget, not guaranteed wall-clock seconds.

The source requires an accumulation host with audio disabled, explicit buffer clearing, correct texture/FBO identifiers, and a reset/sample-count policy. It cannot replace the default temporal-feedback shader by itself. The current preparation script does not build it; see the [branch review](../../documentation/branch-review.md#3-exe_gfx-accumulating-a-still-image) for the adaptation work.
