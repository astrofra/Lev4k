# CMake builds and the 4K limit

**CMake preserves the compact executable pipeline.** The supplied LastFrameBuffer sample builds to **1,724 bytes** in Release through CMake, byte-for-byte identical to the measured Visual Studio project build. CMake organizes compilation and tool execution; the production executable is still linked and compressed by Crinkler.

The existing `Lev4k.sln` and `.vcxproj` remain available. CMake writes its generated shader, objects, projects, reports, and executables under `build/`. It does not regenerate `src/shaders/fragment.inl` or replace the original project's outputs.

## Automatic Windows builds

Install Visual Studio 2022 with the **Desktop development with C++** tools, a Windows SDK, and **CMake 3.24 or newer**. The scripts can find CMake on PATH, in its standard installation directory, or in the Visual Studio installation. They use Windows PowerShell and do not install software.

From a normal PowerShell or command prompt:

| Command | Result |
| --- | --- |
| `build_editor.bat` | Configure and build the interactive Editor with the normal MSVC linker. |
| `build_snapshot.bat` | Build the intro with Crinkler's faster Snapshot compression and check its size. |
| `build_release.bat` | Build the intro with Crinkler's Release compression and check its size. |
| `build_tests.bat` | Build and run the two hidden-window GPU probes through CTest. |
| `build.bat All` | Build Editor, Snapshot, and Release sequentially. |
| `build.bat` | Build Editor by default. |

In PowerShell, prefix a local command with `.` and a backslash, for example:

```powershell
.\build_release.bat
```

The scripts select **Visual Studio 2022 / Win32** and initialize the matching compiler/SDK environment for Crinkler. A Developer PowerShell is not required. They return a nonzero exit code on configuration, compilation, compression, test, or size-check failure. They do not launch the intro or pause for keyboard input.

Outputs are in `build/cmake-windows-x86/out/Release/`:

- `Lev4k-editor.exe`
- `Lev4k-snapshot.exe`
- `Lev4k-release.exe`
- `snapshot-report.html` and `release-report.html`

Run Editor with the repository root as the working directory so it can reload `src/shaders/fragment.frag`. Alternatively, open `build/cmake-windows-x86/Lev4k.sln`, select the `lev4k_editor` startup project, and press F5. The generated solution supplies the working directory. Its CMake configuration is named `Release`; the **target** determines the profile, so `lev4k_editor` still has Editor controls and diagnostics.

## What keeps the executable small?

| Target | Compiler/link pipeline |
| --- | --- |
| `lev4k_editor` | MSVC, normal C++ runtime and linker, Editor controls, shader diagnostics. |
| `lev4k_snapshot` | Size-oriented x86 objects, custom `entrypoint`, direct Crinkler link, instant compression. |
| `lev4k_release` | The same tiny objects, direct Crinkler link, slower compression and the existing Release search settings. |

The tiny targets preserve the important compiler settings, including `/O1`, `/Ob1`, `/Oi`, `/Os`, `/Oy`, `/GS-`, `/GF-`, `/fp:fast`, `/Gz`, and `/Zl`. Crinkler receives the object files directly, uses `/NODEFAULTLIB` and `/ENTRY:entrypoint`, and retains the previous compression options. No ordinary linked executable is inserted before compression, and CMake's default runtime/debug flags are replaced with explicit target profiles.

The implementation uses a CMake object library and explicit build commands for Shader Minifier and Crinkler. These mechanisms allow generation dependencies and the compression step to remain visible to the build system. See the official [object-library documentation](https://cmake.org/cmake/help/latest/command/add_library.html#object-libraries) and [custom-command documentation](https://cmake.org/cmake/help/latest/command/add_custom_command.html).

Crinkler is selected only for the compact targets; it does not replace the installed Microsoft linker. The CMake project rejects x64 and non-MSVC/non-Windows hosts for this implementation. The SDK selected by CMake and the MSVC tool version are passed to the Crinkler environment setup. See [CMake's Windows SDK selection](https://cmake.org/cmake/help/latest/variable/CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION.html).

The generated shader is selected through `LEV4K_SHADER_HEADER`. With no override, the existing Visual Studio project continues to include `src/shaders/fragment.inl`. This changes only the input file location, not the renderer or shader behavior.

## Enforcing the byte limit

The default maximum is **4,095 bytes**, satisfying a requirement of strictly less than 4,096 bytes. Both compact targets run [CheckIntroSize.cmake](../cmake/CheckIntroSize.cmake) on every requested build, even when their executable is already up to date. An oversized file remains available for analysis, but the build fails.

For a competition allowing exactly 4,096 bytes:

```powershell
.\build_release.bat -MaxIntroBytes 4096
```

For strictly less than 4,000 bytes, use `-MaxIntroBytes 3999`. Raising the value for an experiment changes the acceptance limit; it does not make that executable a 4K production. Record the actual file size and competition rules.

The batch scripts default to 4,095 on each invocation. To set the limit when using CMake directly, pass `-DLEV4K_MAX_INTRO_BYTES=<maximum>` at configuration time.

## Direct CMake commands

From the repository root:

```powershell
cmake --preset windows-x86
cmake --build --preset editor
cmake --build --preset snapshot
cmake --build --preset intro
```

For GPU checks:

```powershell
cmake --preset windows-x86 -DLEV4K_BUILD_GPU_TESTS=ON
cmake --build build/cmake-windows-x86 --config Release --target lev4k_feedback_probe_0 lev4k_feedback_probe_1
ctest --test-dir build/cmake-windows-x86 -C Release --output-on-failure
```

Shader generation has explicit file dependencies and runs when its source or tool changes, or its output is missing. CMake therefore needs no separate `EditorNoRecompile` target. Keep build commands against the same build directory sequential; different configurations of the old `.sln` still retain their original shared-directory constraints.

The tested entry point is the `windows-x86` Visual Studio generator preset. Other generators are not part of the validation below.

## Examples and Ubuntu

New [prepared Drifting Shore copies](examples.md) also receive the CMake files and build scripts. Their byte limit is the same by default. That adaptation exceeds 4K, so an exploratory larger build requires an explicit limit such as `build_release.bat -MaxIntroBytes 4500`. The feedback GPU tests are specific to the default sample and are not included in the prepared production copies.

CMake is a useful starting point for a future Ubuntu build, but the application still uses Win32/WGL and WinMM. This change does not implement a Linux platform layer, ELF compression, or a native Linux executable. The [Ubuntu porting study](ubuntu-porting-study.md) still applies to that work.

## Validation

Measured on **19 September 2026** with CMake **3.30.3**, Visual Studio 2022, MSVC **19.41.34120**, and SDK **10.0.22621.0**, on the Windows 11 machine in the [validation record](validation-record.md).

| Check | Result |
| --- | --- |
| Batch build from ordinary PowerShell | Editor, Snapshot, and Release passed. |
| Snapshot | **1,770 bytes**, matching the existing project measurement. |
| Release | **1,724 bytes**, byte-identical to the existing project artifact. |
| GPU tests through CTest | Both passed: mipmaps disabled and enabled. |
| Artificial 1,000-byte limit | Build failed as expected; an immediate repeat with the cached executable also failed. Restoring 4,095 passed. |
| Missing generated shader header | Regenerated automatically; rebuilt Release retained the same SHA-256 and 1,724-byte size. |
| x64 configuration | Rejected with an x86 requirement before building the intro. |
| Prepared Drifting Shore, path containing spaces | Batch-built all targets from outside the copy; Release remained 4,229 bytes with an explicit 4,500-byte study limit. |

Release SHA-256 for both build systems: `4B39DA4348A89872F45BF671E50D6279139D3C6E24B7A8B281F195C2F5581B93`.

These results preserve the existing sample's size and GPU validation. Full audiovisual playback and startup of the exact compressed executable retain the acceptance status recorded in the [LastFrameBuffer guide](last-frame-buffer.md); this build-system change does not add a new runtime compatibility claim.
