# foo_cue_charset

[![Language: C++23](https://img.shields.io/badge/language-C%2B%2B23-00599C.svg)](https://isocpp.org/)
[![Build system: Meson](https://img.shields.io/badge/build%20system-Meson-00ADD8.svg)](https://mesonbuild.com/)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version: 0.1.0](https://img.shields.io/badge/version-0.1.0-informational.svg)](meson.build)

**CUE Charset** is a foobar2000 component for Windows x64 that reads external `.cue` sheets saved
in legacy Cyrillic (and other single-byte) encodings. It converts the CUE text — including the
`FILE` directive — to UTF-8 in memory before the sheet is parsed, so a CUE saved as Windows-1251,
KOI8-R, CP866 or ISO-8859-5 loads as normal tracks with correct metadata and a correctly resolved
audio filename.

> Modern foobar2000 already handles correctly encoded **UTF-8** CUE sheets. This component exists
> only for the legacy case where the `.cue` file is *not* UTF-8 and would otherwise load as
> mojibake (or fail to find a Cyrillic-named audio file). Installing it does not change how valid
> UTF-8 CUE sheets behave.

## Documentation

Start at the [documentation home](docs/README.md), organized around Diátaxis:
[tutorials](docs/README.md#tutorials), [how-to guides](docs/README.md#how-to-guides),
[reference](docs/README.md#reference), and [explanation](docs/README.md#explanation).

## How it works

The component registers a redirecting **input** for the `.cue` extension. When foobar2000 opens a
`.cue`, the component:

1. reads the raw bytes of the file;
2. converts them to UTF-8 using the deterministic algorithm below;
3. parses the resulting UTF-8 with the official foobar2000 CUE parser;
4. exposes one subsong per CUE track and delegates audio decoding of each track's
   `[start, length]` segment to the normal installed decoder (FLAC, WavPack, etc.) via the
   official `input_helper_cue`.

Reading is transparent and read-only: opening or playing a CUE never modifies the original file.

## Supported encodings (v0.1.0)

- ASCII
- UTF-8 without BOM
- UTF-8 with BOM
- UTF-16 little-endian with BOM
- UTF-16 big-endian with BOM
- Windows-1251 *(default legacy fallback)*
- KOI8-R
- CP866 / IBM866
- ISO-8859-5

## Detection algorithm (deterministic)

There are two modes, selectable in the preferences page:

**Automatic** (default):

1. If the file starts with a UTF-8, UTF-16 LE or UTF-16 BE **BOM**, it is decoded as that Unicode
   form and the BOM is removed.
2. Otherwise, the whole file is validated as **strict UTF-8** (pure ASCII counts as UTF-8). If it
   is valid, it is used as-is.
3. Otherwise, the file is decoded with the configured **legacy fallback** encoding.

**Force selected encoding:**

- The whole file is decoded with the selected encoding, regardless of whether the bytes could
  accidentally pass UTF-8 validation. Force means force.

This is **not** statistical language detection. Automatic mode detects Unicode and otherwise uses
the single legacy fallback you configured; it does not guess between Windows-1251, KOI8-R, CP866
and ISO-8859-5. Malformed UTF-8/UTF-16, embedded NUL bytes, oversized files (>16 MiB) and bytes
that are undefined in the selected legacy code page are rejected with a clear error — the text is
never silently replaced with `?` or U+FFFD.

## Installation

Open `foo_cue_charset.fb2k-component` with foobar2000, or install it through:

```text
Preferences -> Components -> Install...
```

Restart foobar2000 when prompted.

## Preferences and defaults

The page lives under:

```text
Preferences -> Tools -> CUE Charset
```

| Setting | Options | Default |
| --- | --- | --- |
| Detection mode | Automatic / Force selected encoding | Automatic |
| Legacy fallback encoding | Windows-1251 / KOI8-R / CP866 / ISO-8859-5 | Windows-1251 |
| Log encoding decisions to the foobar2000 Console | On / Off | Off |

Settings persist across restarts. "Reset page to defaults" restores the values above.

## Decoder priority

The component is registered as a normal-merit input for `.cue`. The SDK places newly discovered
normal-merit inputs at the beginning of the decoder list. If your setup resolves `.cue` to the built-in
handler first, raise this component in:

```text
Preferences -> Playback -> Decoding   (Decoder priority list)
```

so that **CUE Charset** appears above the built-in CUE reader.

## Usage example

A CUE sheet saved as **Windows-1251** (here shown as its UTF-8 equivalent):

```cue
PERFORMER "Август"
TITLE "Демоны любви"
FILE "Август - Демоны любви.flac" WAVE
  TRACK 01 AUDIO
    TITLE "Демоны любви"
    PERFORMER "Август"
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    TITLE "Ночь"
    PERFORMER "Август"
    INDEX 01 04:12:00
```

Dragging this `.cue` into foobar2000 loads two tracks with correct Cyrillic metadata, resolves the
Cyrillic FLAC filename next to the sheet, and plays/seeks through the normal FLAC decoder.

## Build prerequisites

- foobar2000 2.25+ x64 (to run the component).
- Visual Studio 2022 Build Tools with the MSVC v143 toolchain (or newer).
- [Meson](https://mesonbuild.com/) and [Ninja](https://ninja-build.org/).
- [Just](https://github.com/casey/just) for the task recipes.
- [7-Zip](https://www.7-zip.org/) on `PATH` (to unpack the SDK `.7z`).
- `clang-format` and `clang-tidy` for local quality checks.

## SDK setup

`just setup` downloads and unpacks the official foobar2000 SDK and WTL into `subprojects/`, then
configures Meson with `-Dsdk_path=wrap`:

```powershell
just setup
```

Pinned downloads (never committed; ignored under `subprojects/`):

| Dependency | URL | SHA-256 |
| --- | --- | --- |
| foobar2000 SDK 2025-03-07 | <https://www.foobar2000.org/downloads/SDK-2025-03-07.7z> | `ccda3c5840e66e0e28a7e4fe36407c4e78581aa30c40c362a188fcbaae799a3e` |
| WTL 10.0.10320 (headers) | <https://www.nuget.org/api/v2/package/wtl/10.0.10320> | `28670adb25c05772a4b5d7c598c98b1567587b377d689a792845b026768f4d3a` |

WTL is required only because the official SDK helper sources (`cue_parser`, `input_helpers`, …)
include it through their precompiled header. It is a header-only, freely redistributable library.

Catch2 v3 is fetched automatically from Meson WrapDB (`subprojects/catch2.wrap`) on first configure.

## Build, test and package

```powershell
just setup      # resolve SDK/WTL/Catch2 wraps and configure the build
just check      # build with /W4 /WX, verify formatting, run clang-tidy and unit tests
just release    # build Release, run tests, produce foo_cue_charset.fb2k-component
```

Other recipes: `just build-debug`, `just build-release`, `just test`, `just format`,
`just format-check`, `just tidy`, `just package`, `just clean`. `just release` writes
`foo_cue_charset.fb2k-component`, a ZIP archive containing `foo_cue_charset.dll` only.

The SDK-independent encoding module (`src/encoding/`) builds and unit-tests as plain C++ without
the SDK; to run only those tests, configure with `-Dbuild_component=false`.

Additional SDK parser regression tests cover track boundaries, `BINARY` file types, metadata,
and cancellation. They use `shared.dll` from an installed player without launching it:

```powershell
meson configure build '-Dfoobar2000_path=C:/Program Files/foobar2000'
just test
```

The runtime must match the build architecture. These tests supplement the
[manual player test plan](docs/how-to/verify-in-player.md); they do not verify player registration,
playback, decoder priority, or preference persistence. See also the
[SDK feasibility evidence](docs/explanation/sdk-integration.md) and [architecture](docs/explanation/architecture.md).

## Limitations

- The four legacy encodings are not auto-distinguished. In Automatic mode, a non-Unicode CUE is
  always decoded with the one configured fallback; pick the right one or use Force mode.
- Read-only in v0.1.0: the component never writes tags or rewrites the CUE. Tag-write requests
  return foobar2000's normal "unsupported" result.
- Windows x64 only. Uses Win32 code-page conversion APIs (no ICU/iconv runtime dependency).
- Consecutive tracks within one audio source must have strictly increasing `INDEX 01` timestamps;
  equal or decreasing timestamps are rejected.
- `FILE ... BINARY` images use the SDK's raw CD PCM reader and require the exact referenced file.
  Extension substitution is disabled for binary images.
- Conversion checks cancellation periodically. The SDK CUE parser has no cancellation hook;
  cancellation is checked immediately before and after each synchronous parser call.

## Troubleshooting

- **Cyrillic shows as mojibake** — the fallback encoding is wrong. Open
  `Preferences -> Tools -> CUE Charset` and select the encoding the file was actually saved in
  (most Russian CUE sheets are Windows-1251), or switch to Force mode for that encoding. Enable
  "Log encoding decisions to the foobar2000 Console" to see which encoding was used.
- **Tracks load but the audio file is "not found"** — the `FILE` name in the CUE must match the
  real file on disk (including its Cyrillic characters). The component resolves the path with the
  same semantics as foobar2000. As a convenience, if the exact `FILE` target is missing but a file
  with the **same base name and a different audio extension** exists next to it (e.g. the CUE says
  `"album.wav"` but only `album.flac` is present), that file is used automatically. If **several**
  such same-named files exist (e.g. both `album.flac` and `album.ape`), the component refuses to
  guess and reports an ambiguity error — fix the `FILE` line to name the correct file. A genuine
  name mismatch (different base name) still cannot be resolved automatically.
- **The built-in handler loaded the CUE instead** — raise CUE Charset in the decoder priority list
  (see [Decoder priority](#decoder-priority)).

## Privacy

CUE contents are processed entirely locally and are never transmitted anywhere. Diagnostic logging,
when enabled, prints only concise encoding decisions (never file contents or full paths).

## License

MIT — see [LICENSE](LICENSE). Copyright (c) 2026 Aleksandr Pavlov <ckidoz@gmail.com>.

This component links against the official foobar2000 SDK and WTL, which are distributed under their
own licenses and are not redistributed in this repository.
