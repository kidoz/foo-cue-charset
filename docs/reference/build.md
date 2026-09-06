# Build commands, options, and dependencies

Command definitions: [justfile](../../justfile). Target definitions: [meson.build](../../meson.build).
All recipes use PowerShell on Windows. For a task procedure, see
[Build, test, and package](../how-to/build-and-test.md).

## Prerequisites

| Tool or environment | Use |
| --- | --- |
| Windows x64 | Build and target platform |
| Visual Studio 2022 Build Tools, MSVC v143 or newer, Windows SDK | C++ compilation and Windows resources; SDK helper builds also require the ATL headers |
| Meson and Ninja | Configure and execute builds; `--vsenv` activates Visual Studio |
| Just | Run project recipes |
| 7-Zip (`7z` on PATH) | Extract SDK and WTL downloads |
| clang-format and clang-tidy | Formatting and static analysis |
| Network access on first setup | Obtain pinned SDK, WTL, and Catch2 dependencies |
| foobar2000 2.25+ x64 | Run the component; optional parser tests load its matching `shared.dll` |

## Just recipes

| Recipe | Effect |
| --- | --- |
| `setup` | Run `sdk-wrap`; configure `build_dir` with Visual Studio and `sdk_path` |
| `reconfigure` | Run `sdk-wrap`; regenerate an existing `build_dir` |
| `build` | Configure if needed, then compile `build_dir`; assumes SDK preparation is already available |
| `build-debug` | Prepare SDK and compile `build_dir`; preserves an existing directory's build type |
| `build-release` | Prepare SDK and compile `release_dir`; selects Release when creating that directory |
| `test` | Build `build_dir`, then run its tests |
| `test-release` | Build `release_dir`, then run its tests |
| `format` | Build, then apply clang-format to the configured project file list |
| `format-check` | Build, then check formatting without edits |
| `tidy` | Build, then run clang-tidy on the configured source list |
| `check` | Build, format-check, tidy, and test |
| `package` | Build and archive the default-build DLL |
| `package-release` | Build and archive the Release DLL |
| `release` | Build Release, run its tests, and package it |
| `clean` | Recursively remove the configured build and release directories |
| `sdk-wrap` | Verify SDK/WTL archive hashes, extract if needed, and copy project Meson package files |
| `sdk` | Standalone SDK download/extraction under `third_party`; not used by the normal build and does not perform the `sdk-wrap` hash check |

Recipe variables default to `build_dir=build`, `release_dir=build-release`, and `sdk_path=wrap`.
Pass overrides before the recipe, for example `just build_dir=build-local check`. Both packaging
recipes overwrite the root `foo_cue_charset.fb2k-component` with the selected build's DLL.
A fresh normal build uses Meson's default Debug configuration; existing directories retain their
configuration unless explicitly reconfigured.

## Meson options

Defined in [meson_options.txt](../../meson_options.txt):

| Option | Default | Meaning |
| --- | --- | --- |
| `sdk_path` | `wrap` | Use the prepared SDK subproject, or supply an extracted SDK path |
| `build_component` | `true` | Build the DLL; `false` leaves the SDK-independent tests |
| `foobar2000_path` | Empty | With the component enabled, supplying a directory containing matching `shared.dll` enables SDK parser tests |

Each build directory has its own options. A configured `foobar2000_path` in `build/` does not
configure `build-release/`. Its DLL is added to the test process's PATH, not bundled in the package.

## Test targets

| Target | Scope |
| --- | --- |
| `test_utf` | BOM recognition, strict UTF validation/conversion, NUL detection, cancellation |
| `test_decoder` | Detection policy, legacy code pages, errors, chunking, cancellation |
| `test_cue_text` | Complete in-memory CUE conversion and line preservation |
| `test_path_util` | SDK-independent path operations |
| `test_cue_sheet` | Optional SDK parsing, FILE types, track boundaries, metadata, abort propagation |

## Dependency pins

The recipe and wrap files are authoritative if pins change. Downloaded archives and extracted
third-party trees are ignored by Git.

| Dependency | Download | SHA-256 |
| --- | --- | --- |
| foobar2000 SDK 2025-03-07 | [Official SDK archive](https://www.foobar2000.org/downloads/SDK-2025-03-07.7z) | `ccda3c5840e66e0e28a7e4fe36407c4e78581aa30c40c362a188fcbaae799a3e` |
| WTL 10.0.10320 headers | [NuGet archive](https://www.nuget.org/api/v2/package/wtl/10.0.10320) | `28670adb25c05772a4b5d7c598c98b1567587b377d689a792845b026768f4d3a` |
| Catch2-3.16.0 | [Source archive](https://github.com/catchorg/Catch2/archive/v3.16.0.tar.gz) | `0957cae5821b17ce07f0833aaa52b5137643a8382203221f363a8303c109af34` |

Pins are stored in [justfile](../../justfile), [fb2k-sdk.wrap](../../subprojects/fb2k-sdk.wrap), and
[catch2.wrap](../../subprojects/catch2.wrap). The SDK archive lives in `subprojects/packagecache/`
and is extracted to `subprojects/fb2k-sdk/`. The `sdk-wrap` recipe places WTL headers under that
SDK tree and copies the project-owned package files into it.

## Compiler and quality policy

Component code uses C++ latest/C++23, `/W4 /WX /permissive- /EHsc /utf-8`, and Control Flow Guard
compile/link flags for the DLL. SDK sources use C++20 and relaxed warnings. Third-party angle-bracket
headers are separated from component warnings with MSVC external-header flags.

The [clang-tidy configuration](../../.clang-tidy) treats enabled diagnostics as errors. Its
single-line trailing-comma policy preserves commas that clang-format moves with an initializer,
preventing repeated contradictory rewrites. The formatting list includes the Cyrillic fixture header.

## Artifacts

| Path | Contents |
| --- | --- |
| `build/foo_cue_charset.dll` | Default-build component |
| `build-release/foo_cue_charset.dll` | Release component |
| `foo_cue_charset.fb2k-component` | ZIP archive with exactly `foo_cue_charset.dll` |
| `<build-dir>/meson-logs/testlog.txt` | Test results for that build |
| `<build-dir>/compile_commands.json` | Compiler database used by clang-tidy |

[Documentation home](../README.md)
