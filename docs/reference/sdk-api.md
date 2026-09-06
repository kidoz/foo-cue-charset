# SDK integration APIs and source map

Baseline: official foobar2000 SDK **2025-03-07**, inspected under
`subprojects/fb2k-sdk/`. Header paths below are relative to that SDK root. The declarations
record the interfaces used by this component; abbreviated parameter lists are descriptive,
not standalone C++ examples. Download pins are in the [build reference](build.md#dependency-pins).

## Public interfaces

- `foobar2000/SDK/input.h`
  - `class NOVTABLE input_entry : public service_base` —
    `virtual bool is_our_path(const char * p_full_path, const char * p_extension) = 0;`
    decides which extension we claim (must use only the path/extension, no file access).
  - `enum { flag_redirect = 1, ... }; virtual unsigned get_flags() = 0;` —
    `flag_redirect` marks an input that opens **another** input for decoding, "used to avoid
    circular call possibility".
  - `class input_entry_v2 : public input_entry` — `get_guid()`, `get_name()`,
    `get_preferences_guid()`, `bool is_low_merit()` (decoder-priority identity/order).
- `foobar2000/SDK/input_impl.h`
  - `class input_impl` — the multi-subsong implementation surface we implement:
    `open(service_ptr_t<file>, const char*, t_input_open_reason, abort_callback&)`,
    `get_subsong_count()`, `get_subsong(unsigned)`,
    `get_info(t_uint32, file_info&, abort_callback&)`,
    `get_stats2(uint32_t, abort_callback&)`,
    `decode_initialize(t_uint32, unsigned, abort_callback&)`, `decode_run(audio_chunk&, …)`,
    `decode_seek(double, …)`, `decode_can_seek()`.
  - `class input_stubs` — supplies stub implementations of the optional methods
    (`decode_run_raw`, `set_logger`, `flush_on_pause`, `extended_param`, dynamic info, …) plus the
    interface typedefs; we inherit from it.
  - `template<typename T, unsigned t_flags = 0> class input_factory_t` — registration template.
  - `input_open_file_helper(service_ptr_t<file>&, const char*, t_input_open_reason, abort_callback&)`
    — opens the file with the right access mode if no hint was provided.
- `foobar2000/helpers/cue_parser.h` (official CUE grammar parser — reused, not reimplemented)
  - `void cue_parser::parse(const char* p_cuesheet, t_cue_entry_list& p_out);` — throws
    `cue_parser::exception_bad_cuesheet` (derived from `exception_io_data`) on failure.
  - `struct cue_entry { pfc::string8 m_file, m_fileType; unsigned m_track_number;
    t_cuesheet_index_list m_indexes; };`
  - `void cue_parser::parse_info(const char* p_cuesheet, file_info& p_info, unsigned p_index);` —
    fills album-level + track-level metadata for a 1-based track number.
- `foobar2000/helpers/cuesheet_index_list.h`
  - `struct t_cuesheet_index_list { double m_positions[100]; double start() const {return
    m_positions[1];} double pregap() const {return m_positions[1]-m_positions[0];} };`
- `foobar2000/helpers/input_helper_cue.h` / `.cpp`
  - `void input_helper_cue::open(service_ptr_t<file> p_filehint, const playable_location&
    p_location, unsigned p_flags, abort_callback&, double p_start, double p_length, bool binary);`
    Opens the referenced audio through `input_helper` (which calls
    `input_entry::g_open_for_decoding(..., from_redirect=true)`), seeks to `p_start`, and bounds
    the segment. `p_length > 0` is an exact length; `p_length <= 0` means "to the end of the
    referenced audio" (`m_length = ref_length - m_start + p_length`).
  - `bool run(audio_chunk&, abort_callback&)`, `void seek(double, abort_callback&)`,
    `bool can_seek()` — segment decode with trimming.
- `foobar2000/SDK/filesystem.h`
  - `static bool filesystem::g_relative_path_parse(const char* p_relative_path, const char*
    p_playlist_path, pfc::string_base& out);` — resolves a CUE `FILE` directive relative to the
    `.cue` path with the official semantics. `g_get_canonical_path()` is the fallback for
    already-absolute paths.
- `foobar2000/SDK/file.h`
  - `t_size file::read(void*, t_size, abort_callback&);` (used for the bounded chunked read)
  - `t_filestats2 file::get_stats2_(uint32_t, abort_callback&);`
- `foobar2000/SDK/playable_location.h`
  - `playable_location make_playable_location(const char* path, t_uint32 subsong);`
- `foobar2000/SDK/componentversion.h`
  - `DECLARE_COMPONENT_VERSION(NAME, VERSION, ABOUT)` and
    `VALIDATE_COMPONENT_FILENAME("foo_cue_charset.dll")`.
- `foobar2000/SDK/preferences_page.h`
  - `class preferences_page_v3 : public preferences_page_v2` with
    `preferences_page_instance::ptr instantiate(fb2k::hwnd_t parent,
    preferences_page_callback::ptr)`; parent branch `preferences_page::guid_tools`.
- `foobar2000/SDK/cfg_var.h`
  - `cfg_var_modern::cfg_int` / `cfg_bool` — thread-safe (atomic) persisted configuration backed
    by the configStore.

### Binary metadata

`foobar2000/helpers/input_helper_cue.h` also declares:

```cpp
static void get_info_binary(const char* path, file_info& out, abort_callback& abort);
```

The component pairs this with `input_helper_cue::open(..., binary=true)` for raw CD PCM.
For non-binary sources it uses `input_helper::g_get_info(..., p_from_redirect=true)` and
`open(..., binary=false)`. The helper computes non-positive lengths as
`source_length - start + supplied_length`; the component supplies zero only for a terminal track.

## Linked helper sources

Compiled into the SDK static libraries (see `subprojects/packagefiles/fb2k-sdk/meson.build`):

| Helper source | Why it is needed |
| --- | --- |
| `helpers/cue_parser.cpp`, `cue_creator.cpp`, `cuesheet_index_list.cpp` | Official CUE grammar parser (`parse`, `parse_info`) and the index-list type. |
| `helpers/input_helpers.cpp` | `input_helper` — opens the referenced audio via a recursion-safe redirect. |
| `helpers/input_helper_cue.cpp` | `input_helper_cue` — segment `[start, length]` decode + seek/trim. |
| `helpers/file_list_helper.cpp`, `readers.cpp` | Symbols referenced by `input_helpers.cpp` (`fileCreateReadAhead`, `createFileMemMirrorAsync`). |

`helpers/cue_parser_embedding.cpp` is not linked by the default wrap build; only the explicit
`-Dsdk_path=<dir>` branch of the root `meson.build` still compiles it.

`pfc/pfc-fb2k-hooks.cpp` is intentionally **omitted**: it only re-exports `pfc::crashHook` /
`pfc::winFormatSystemErrorMessageHook`, which `SDK/utility.cpp` also defines; compiling both causes
`LNK2005` once the helpers pull in `utility.cpp`.

The official helpers include WTL through their precompiled header, so WTL headers are placed under
`subprojects/fb2k-sdk/wtl/` by `just sdk-wrap` and added to the SDK include path.

## Component source map

| File | Warnings | Role |
| --- | --- | --- |
| `src/encoding/utf.*`, `decoder.*` | `/W4 /WX` | SDK-independent encoding module. |
| `src/path_util.*` | `/W4 /WX` | SDK-independent path helpers for the FILE resolver (unit-tested). |
| `src/component.cpp` | `/W4 /WX` | `DECLARE_COMPONENT_VERSION` + filename validation. |
| `src/cue_sheet.*` | `/W4 /WX` | SDK parsing, FILE types, timestamp validation, and cancellation boundaries. |
| `src/cue_input.cpp` | `/W4 /WX` | The `.cue` input logic. |
| `src/config.cpp`, `src/preferences.cpp` | `/W4 /WX` | Config persistence + preferences page. |
| `src/cue_input_register.cpp` | relaxed (SDK level) | Only the `input_factory_t<>` registration; isolated because instantiating the SDK input template emits C4100 from SDK headers in NDEBUG builds. Pulled in with `link_whole` so the static service-factory initializer is not stripped. |

Angle-bracket (`<…>`) third-party headers are compiled with `/external:anglebrackets
/external:W0`, so the SDK/WTL/Windows/Catch2 headers do not pollute the strict component net.

## Tests

The SDK-independent suites compile conversion and path code without foobar2000.
`test_cue_sheet` additionally links the SDK and loads the matching player's `shared.dll`.
It tests parser wrappers and callback behavior without launching the player.

For the service-selection rationale and acceptance boundary, see
[Why a redirecting input](../explanation/sdk-integration.md).

[Documentation home](../README.md)
