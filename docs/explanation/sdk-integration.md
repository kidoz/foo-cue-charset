# Why the component uses a redirecting input

The selected integration route is an SDK input service for external `.cue` files. It reads
original bytes, exposes multiple subsongs, and opens a referenced audio source for each segment.
This design uses the public foobar2000 SDK 2025-03-07 APIs listed in the
[SDK reference](../reference/sdk-api.md).

## Why an input fits CUE tracks

The inspected SDK's playlist-processing path tries playlist loaders first, then falls through
to track indexing. That path opens an input for information and enumerates its subsongs.
An input can therefore supply the per-track metadata and the decode behavior for a CUE entry.

A playlist loader alone emits locations without implementing the bounded decoder behavior for
each track. The SDK's `input_helper_cue` already supports opening an audio source, seeking to a
start position, and trimming at the segment end. Combining it with a multi-subsong input provides
the required behavior without creating a private core interface.

## Why delegation does not recurse

The factory registers `input_entry::flag_redirect`. Normal audio probes use
`p_from_redirect=true`; the segment helper also opens normal sources as a redirect. The SDK's
input-opening path excludes redirect services in that situation, preventing a source reference
from recursively reopening the CUE handler. Binary sources use the SDK PCM reader directly.

## Priority is a player setting

`input_entry_v2` supplies a GUID, display name, preference-page GUID, and a merit flag for the
decoder table. The inherited normal merit places a newly discovered decoder at the beginning
of that list. Existing user ordering still matters, so the installation guide includes the
[priority adjustment](../how-to/install-and-configure.md#set-handler-priority).

The component processes valid Unicode CUEs itself once selected. It does not need to dispatch
the same CUE to a second reader, which would complicate priority and recursion handling.

## What the evidence establishes

| Required capability | Evidence in the implementation |
| --- | --- |
| Claim `.cue` | `g_is_our_path` and the registered input factory |
| Read original bytes | `input_open_file_helper` followed by the bounded byte reader |
| Expose multiple tracks | SDK parsing, track records, and subsong enumeration |
| Delegate audio safely | Redirect-aware probes and `input_helper_cue` |
| Participate in ordering | Stable input GUID, name, and normal merit |

Builds and SDK parser tests establish source-level compatibility and local parsing behavior.
They do not prove that a running player selects the component or that playback, seeking, and
preferences work through its core services. Those checks remain pending in the
[acceptance matrix](../reference/player-test-matrix.md).

## Why there is no automatic on-disk conversion

The public SDK supplies an integration route, so the last-resort design that explicitly converts
selected files on disk has not been implemented. The reader remains read-only. No player
patching, private APIs, Win32 file hooks, or automatic rewrite on scanning is used.

[Documentation home](../README.md)
