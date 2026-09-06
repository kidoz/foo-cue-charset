# Behavior, preferences, and limits

Source of truth: [configuration](../../src/config.hpp), [decoder](../../src/encoding/decoder.cpp),
[CUE input](../../src/cue_input.cpp), and [track parsing](../../src/cue_sheet.cpp).

## Platform and identity

| Item | Value |
| --- | --- |
| Component name | CUE Charset |
| Version | 0.1.0 |
| Target player | foobar2000 2.25+ x64 on Windows |
| DLL | `foo_cue_charset.dll` |
| Package | `foo_cue_charset.fb2k-component`, containing the DLL only |
| License | [MIT](../../LICENSE) |

## Preferences

Location: **Preferences → Tools → CUE Charset**.

| Setting | Values | Default |
| --- | --- | --- |
| Detection mode | Automatic; Force selected encoding | Automatic |
| Legacy fallback encoding | Windows-1251; KOI8-R; CP866; ISO-8859-5 | Windows-1251 |
| Log encoding decisions to the foobar2000 Console | On; Off | Off |

Settings use the SDK configStore. Reset restores the controls to defaults; Apply saves them.
Invalid persisted values are read as defaults.

## Encoding selection

| Mode and input | Result |
| --- | --- |
| Automatic, UTF-8 BOM | Remove BOM; validate and use UTF-8 |
| Automatic, UTF-16 LE or BE BOM | Remove BOM; validate and convert that byte order |
| Automatic, no BOM, strict UTF-8 (including ASCII) | Use UTF-8 |
| Automatic, no BOM, invalid UTF-8 | Decode with the selected legacy fallback |
| Force selected encoding | Decode all bytes using the selected legacy encoding, including BOM bytes |

A recognized Unicode BOM with a malformed body is an error. Without a BOM, malformed-looking
UTF-8 may instead be valid legacy input and is processed by the configured fallback.

The supported legacy Windows code pages are 1251, 20866 (KOI8-R), 866, and 28595 (ISO-8859-5).
Windows-1251 byte `0x98` is explicitly rejected; it is not rejected solely by Win32's flags.

The complete document is converted before parsing. Line endings, whitespace, quotes, metadata,
and referenced paths are preserved apart from encoding conversion and recognized BOM removal.

## Referenced audio

- References are resolved relative to the CUE using the SDK filesystem path API, with a
  canonical-path fallback and an explicit CUE-directory retry.
- An existing exact target wins. For non-binary input, a missing target can resolve to exactly
  one same-basename file with another supported candidate extension.
- Candidate extensions: `.flac`, `.wv`, `.ape`, `.wav`, `.mp3`, `.m4a`, `.mp4`, `.aiff`, `.aif`.
  More than one candidate produces an ambiguity error.
- `BINARY` references require the exact file and use the SDK's stereo 16-bit, 44.1 kHz raw PCM
  reader. Other references use installed decoders.
- Missing/unreadable sources can remain listed without a known source duration; playback fails
  when the source cannot be opened.

## Tracks and writes

One audio CUE track becomes one subsong, identified by its CUE track number. Playback starts at
`INDEX 01`; adjacent tracks in the same source must have strictly increasing starts. Each
nonterminal track ends at the next start, and a terminal track runs to the source's end.
Conflicting binary/decoded declarations of one resolved source are rejected.

The component is read-only. Opening, scanning, and playing a CUE never rewrite it. Tag-write
requests return the SDK's unsupported-tagging result.

## Limits and privacy

| Constraint | Value or behavior |
| --- | --- |
| Maximum input | 16 MiB, configured in code; oversized input is rejected |
| Text validation | Strict Unicode, no embedded NUL, no silent invalid-character replacement |
| Conversion polling | 16 KiB processing chunks; finish a Unicode scalar before polling |
| SDK parsing | Synchronous; cancellation checked before and after each call |
| Processing | Local; CUE contents are not transmitted |
| Optional Console logging | Encoding decisions and concise errors; no full document or default private-path logging |

These are implementation contracts. Player acceptance status is recorded separately in the
[test matrix](player-test-matrix.md).

[Documentation home](../README.md)
