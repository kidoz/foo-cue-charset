# Player acceptance matrix

These are expected results for foobar2000 2.25+ x64. All unchecked or Pending cases remain
**unexecuted**; the matrix is not a record of successful player validation. Automated tests do
not change these statuses.

Use [Run the manual acceptance checks](../how-to/verify-in-player.md) to prepare fixtures,
record the build/profile, and capture results. Test data must use the encoding and source format
specified in each row. Mark each case PASS, FAIL, or NOT RUN with a reason in the run record.

## Environment / lifecycle

| # | Case | Expected result | Status |
| --- | --- | --- | --- |
| 1 | Install `.fb2k-component` on foobar2000 2.25+ x64 (clean profile) | Installs without filename/version validation error; component listed under Preferences → Components | ☐ |
| 2 | Install on an existing profile | Same as above; no conflict with the built-in CUE handler | ☐ |
| 3 | Restart after install | Component loads; `Preferences → Tools → CUE Charset` page present | ☐ |
| 4 | Uninstall | Component removed cleanly; `.cue` files revert to built-in handling | ☐ |

## Encoding / metadata

| # | Case | Expected result | Status |
| --- | --- | --- | --- |
| 5 | Add a UTF-8 (no BOM) CUE | Loads; correct Cyrillic metadata; identical to built-in behavior | ☐ |
| 6 | Add a UTF-8 (BOM) CUE | Loads; BOM not shown in any field | ☐ |
| 7 | Add a Windows-1251 CUE (Automatic mode, default fallback) | Correct Cyrillic album/track metadata | ☐ |
| 8 | Windows-1251 CUE with a Cyrillic `FILE` name | Referenced Cyrillic audio file resolves and is found | ☐ |
| 9 | Switch fallback to KOI8-R, add a KOI8-R CUE | Correct metadata | ☐ |
| 10 | Switch fallback to CP866, add a CP866 CUE | Correct metadata | ☐ |
| 11 | Switch fallback to ISO-8859-5, add an ISO-8859-5 CUE | Correct metadata | ☐ |
| 12 | Force mode + wrong encoding | Mojibake (documents that Force overrides detection); switching back fixes it | ☐ |
| 13 | Enable Console logging, reload a legacy CUE | Console shows `foo_cue_charset: decoded external CUE as <encoding> using automatic fallback` | ☐ |
| 14 | Malformed CUE / undefined legacy bytes | Clean error in Console; no crash; no `?`/U+FFFD substitution | ☐ |
| U1 | UTF-16 LE BOM CUE | Correct metadata and filename after conversion; BOM removed | Pending |
| U2 | UTF-16 BE BOM CUE | Same result as the LE fixture | Pending |

## CUE / audio behavior

| # | Case | Expected result | Status |
| --- | --- | --- | --- |
| 15 | Single-file, multi-track CUE | All tracks listed with correct durations | ☐ |
| 16 | Multi-`FILE` CUE | Each track maps to its own referenced file | ☐ |
| 17 | Playback of a track | Plays through the normal decoder | ☐ |
| 18 | Seek within a track | Sample-accurate seek; stops at the track boundary | ☐ |
| 19 | `INDEX 00` + `INDEX 01` (pregap) | Track starts at `INDEX 01` | ☐ |
| 20 | FLAC / APE / WAV / WavPack sources (where decoders installed) | All play via their decoders | ☐ |
| 21 | Missing referenced audio | Track still listed; playback fails gracefully (dead item), no crash | ☐ |
| 22 | Add file / Add folder / Media Library scan | CUE tracks indexed; no duplicate error popups during scan | ☐ |
| 23 | ReplayGain scan over CUE tracks | Completes via the normal decoder | ☐ |
| 24 | Converter on CUE tracks | Converts via the normal decoder | ☐ |
| 25 | Long paths / network path (where available) | Behaves as normal foobar2000 paths | ☐ |

## Reference resolution (extension fallback)

| # | Case | Expected result | Status |
| --- | --- | --- | --- |
| R1 | `FILE "x.wav"` but only `x.flac` exists next to the CUE | Track resolves to `x.flac` and plays; with logging on, Console shows `resolved missing CUE FILE target by using same-basename .flac file` | ☐ |
| R2 | `FILE "x.wav"` and the exact `x.wav` exists | Uses `x.wav`; no fallback, no log line | ☐ |
| R3 | `FILE "x.wav"` missing and BOTH `x.flac` and `x.ape` exist | CUE fails to load with an actionable ambiguity error listing the candidate extensions; no silent guess | ☐ |
| R4 | `FILE` names a genuinely different base name that does not exist | Track listed but unresolved (dead item); not auto-resolved | ☐ |

## Read-only / persistence / priority

| # | Case | Expected result | Status |
| --- | --- | --- | --- |
| 26 | Attempt to write tags to a CUE track | foobar2000 reports it as read-only/untaggable; original `.cue` byte-for-byte unchanged | ☐ |
| 27 | Confirm CUE unchanged after open + play | File hash identical to before | ☐ |
| 28 | Change preferences, restart, reopen page | Settings persisted | ☐ |
| 29 | Decoder priority list | **CUE Charset** can be ordered above the built-in CUE reader; deterministic which handler wins | ☐ |
| 30 | No handler recursion | No infinite loop / repeated open of the same `.cue`; single load | ☐ |

## Regression cases (player execution pending)

| Case | Expected result | Status |
| --- | --- | --- |
| Two tracks in one FILE start at 01:00:00 then 00:30:00, or both at 01:00:00 | Clean failure; no track silently decodes to EOF | Pending |
| Two different FILE targets both start at 00:00:00 | Valid tracks with independently resolved durations | Pending |
| Raw stereo 16-bit, 44.1 kHz PCM image referenced with BINARY, two tracks | Correct durations, playback, seek, and Converter output through the SDK binary reader | Pending |
| BINARY image is missing but a same-basename FLAC exists | Missing-file error; compressed bytes are never played as PCM | Pending |
| Windows-1251 TITLE contains byte 0x98, in Automatic or Force mode | Clean conversion error; no U+0098 substitution | Pending |
| Cancel while adding a large CUE or querying metadata | Cancellation propagates without being turned into a decode error; a running SDK parse finishes before abort is observed | Pending |

- Cases 5–6 verify that installing the component does not regress valid UTF-8 CUE behavior.
- Case 27 is the core read-only guarantee; verify with a file hash before/after.

[Documentation home](../README.md)
