#pragma once

#include "encoding/abort.hpp"

// Public API of the SDK-independent encoding module. Converts the raw bytes of an external
// CUE sheet to UTF-8 according to an explicit, reproducible policy. No <windows.h> here so
// the header stays usable from plain-C++ unit tests; the Win32 code-page work lives in
// decoder.cpp.

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>

namespace foo_cue_charset::encoding {

//! Supported encodings. The four legacy single-byte pages are the ones offered in
//! the preferences dropdown; the Unicode forms are produced by detection.
enum class text_encoding : std::uint8_t {
  utf8,
  utf16_le,
  utf16_be,
  windows_1251,
  koi8_r,
  cp866,
  iso_8859_5,
};

enum class detection_mode : std::uint8_t {
  //! Detect a BOM, else validate as strict UTF-8, else decode with the selected legacy page.
  automatic,
  //! Decode with the selected encoding regardless of accidental UTF-8 validity. Force is force.
  force_selected,
};

//! Conservative cap on external CUE size. Inputs larger than this are rejected before
//! allocation. 16 MiB is far beyond any real CUE sheet.
inline constexpr std::size_t default_maximum_input_bytes = std::size_t{16} * 1024 * 1024;

struct decode_options {
  detection_mode mode = detection_mode::automatic;
  text_encoding selected_legacy_encoding = text_encoding::windows_1251;
  std::size_t maximum_input_bytes = default_maximum_input_bytes;
};

struct decode_result {
  std::string utf8;
  text_encoding source_encoding = text_encoding::utf8;
  bool had_bom = false;
  bool used_legacy_fallback = false;
};

enum class decode_error_code : std::uint8_t {
  input_too_large,
  invalid_utf8,
  invalid_utf16,
  unsupported_encoding,
  embedded_nul,
  conversion_failed,
};

//! Error carrying enough information for a useful Console/popup message. The message is
//! sanitized: it never contains CUE contents or filesystem paths.
struct decode_error {
  decode_error_code code = decode_error_code::conversion_failed;
  std::string message;
};

//! Converts the whole input to UTF-8 (BOM removed) following the configured policy.
//! On success the result's utf8 string is BOM-free and contains no embedded NUL.
//! check_abort is polled during processing; its exceptions are never mapped to decode errors.
[[nodiscard]] std::expected<decode_result, decode_error>
decode(std::span<const std::byte> input, const decode_options& options, const abort_check& check_abort = {});

//! Human-readable name for UI / Console logging, e.g. "Windows-1251". Never null.
[[nodiscard]] const char* display_name(text_encoding enc) noexcept;

//! True for the four legacy single-byte code pages.
[[nodiscard]] bool is_legacy_encoding(text_encoding enc) noexcept;

} // namespace foo_cue_charset::encoding
