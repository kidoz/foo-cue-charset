# Encoding API

Namespace: `foo_cue_charset::encoding`. Public headers are SDK-independent and Windows-header-free.
Implementation: [decoder.cpp](../../src/encoding/decoder.cpp), [utf.cpp](../../src/encoding/utf.cpp).

## Decode a document

Declared in [decoder.hpp](../../src/encoding/decoder.hpp):

```cpp
std::expected<decode_result, decode_error>
decode(std::span<const std::byte> input, const decode_options& options,
       const abort_check& check_abort = {});
```

| `decode_options` field | Type | Default |
| --- | --- | --- |
| `mode` | `detection_mode` | `automatic` |
| `selected_legacy_encoding` | `text_encoding` | `windows_1251` |
| `maximum_input_bytes` | `std::size_t` | `default_maximum_input_bytes` (16 MiB) |

`text_encoding` values: `utf8`, `utf16_le`, `utf16_be`, `windows_1251`, `koi8_r`, `cp866`,
`iso_8859_5`. `detection_mode` values: `automatic`, `force_selected`.

The API can explicitly force Unicode forms; the preferences UI offers only the four legacy
encodings. In forced Unicode decoding, a matching BOM is removed. Forced legacy decoding treats
BOM bytes as input data. Automatic mode follows the [selection table](behavior.md#encoding-selection).

| `decode_result` field | Meaning |
| --- | --- |
| `utf8` | Owned converted document; no embedded NUL |
| `source_encoding` | Encoding used |
| `had_bom` | A matching Unicode BOM was recognized and removed |
| `used_legacy_fallback` | Automatic detection reached the configured fallback |

`decode_error` contains a `code` and a sanitized `message`. Error codes are `input_too_large`,
`invalid_utf8`, `invalid_utf16`, `unsupported_encoding`, `embedded_nul`, and `conversion_failed`.
Malformed-input failures use `std::unexpected`; allocation failures and callback exceptions can
still throw. The input span is borrowed only for the duration of the call.

## Cancellation

[abort.hpp](../../src/encoding/abort.hpp) defines `abort_check` as `std::function<void()>` and
`processing_chunk_bytes` as 16 KiB. An empty callback disables cancellation checks. A supplied
callback signals cancellation by throwing; its exception propagates unchanged, including
`exception_aborted` when called from the component.

## Unicode primitives

Declared in [utf.hpp](../../src/encoding/utf.hpp):

| Function | Contract |
| --- | --- |
| `detect_bom(bytes)` | Returns `bom_info {type, length}` for UTF-8, UTF-16 LE/BE, or no BOM; `noexcept` |
| `is_valid_utf8(bytes, check_abort = {})` | Strict scalar validation; accepts ASCII and empty input |
| `contains_nul(text, check_abort = {})` | Finds an embedded U+0000 |
| `utf16_to_utf8(bytes, big_endian, out, check_abort = {})` | Validates and converts UTF-16; returns `false` on malformed input |

`utf16_to_utf8` clears and rebuilds `out`; failure or cancellation may leave a partial result.
Callers must consume it only on success. The document-level `decode` function applies the input
limit and NUL policy around these primitives.

`display_name(encoding)` returns an English UI label. `is_legacy_encoding(encoding)` identifies
the four supported legacy code pages. Both are `noexcept`.

[Documentation home](../README.md)
