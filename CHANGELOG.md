# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

### Fixed

- Reject equal or decreasing track timestamps instead of decoding the previous track to EOF.
- Preserve `BINARY` CUE references for SDK raw-PCM metadata and playback; disable extension
  substitution for these images.
- Reject the undefined Windows-1251 byte `0x98`, which Windows NLS otherwise accepts.
- Poll cancellation during encoding conversion and before/after SDK metadata and track parsing.

### Added

- Regression tests for malformed boundaries, binary types, undefined bytes, and cancellation.
  SDK parser tests can be enabled with the `foobar2000_path` Meson option.
- Diátaxis documentation under `docs/`: a guided tutorial, task guides, technical reference,
  design explanations, and a player acceptance matrix.
- Include the Cyrillic fixture header in the formatting check.
- Treat enabled clang-tidy diagnostics as errors and resolve the reported project warnings.

## [0.1.0] - 2026-06-23

Initial release.

### Added

- Transparent reading of external `.cue` sheets saved in legacy encodings, implemented as a
  redirecting foobar2000 `input` for the `.cue` extension. The CUE text (including the `FILE`
  directive) is converted to UTF-8 in memory, parsed with the official `cue_parser`, and each
  track's audio segment is decoded through the normal installed decoders via `input_helper_cue`.
- SDK-independent encoding module (`src/encoding/`) with a deterministic policy:
  - **Automatic** mode: detect a UTF-8/UTF-16 BOM, else validate as strict UTF-8 (ASCII included),
    else decode with the configured legacy fallback.
  - **Force selected encoding** mode.
  - Supported encodings: ASCII, UTF-8 (with/without BOM), UTF-16 LE/BE (with BOM), Windows-1251
    (default fallback), KOI8-R, CP866/IBM866, ISO-8859-5.
  - Strict validation: malformed UTF-8/UTF-16, encoded surrogates, scalars above U+10FFFF,
    embedded NUL, oversized input (>16 MiB) and bytes undefined in the selected code page are
    rejected with a clear error; text is never silently replaced with `?` or U+FFFD.
- Preferences page under **Tools → CUE Charset**: detection mode, legacy fallback encoding,
  optional Console logging of encoding decisions. Settings persist via the configStore.
- Catch2 v3 unit tests for BOM detection, strict UTF-8/UTF-16 validation, all legacy code pages,
  detection/fallback flags, and full in-memory CUE documents.
- Meson + Ninja + Just build with pinned foobar2000 SDK `2025-03-07` and WTL wraps,
  `clang-format`/`clang-tidy` gates, and `.fb2k-component` packaging (DLL only).

### Known limitations

- The four legacy encodings are not auto-distinguished; Automatic mode uses the single configured
  fallback. There is no statistical language detection.
- Read-only: the component never writes tags or rewrites the CUE; tag-write requests return
  foobar2000's normal "unsupported" result.
- Windows x64 only.
- Interactive foobar2000 behaviors are covered by the manual test plan
  (`docs/reference/player-test-matrix.md`) and have not been executed in an automated
  environment.

[0.1.0]: https://github.com/kidoz/foo-cue-charset/releases/tag/v0.1.0
