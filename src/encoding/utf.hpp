#pragma once

// SDK-independent, Windows-independent Unicode primitives for the CUE Charset component.
// Keep this header free of <windows.h> and foobar2000 includes so the encoding module can
// be unit-tested as plain C++.

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace foo_cue_charset::encoding {

//! Byte-order marks we recognize at the start of an external CUE sheet.
enum class bom_type {
  none,
  utf8,     // EF BB BF
  utf16_le, // FF FE
  utf16_be, // FE FF
};

struct bom_info {
  bom_type type = bom_type::none;
  std::size_t length = 0; // number of leading bytes occupied by the BOM
};

//! Identifies a leading BOM without consuming it. Never reads past the buffer.
[[nodiscard]] bom_info detect_bom(std::span<const std::byte> bytes) noexcept;

//! Strict UTF-8 validation. Rejects overlong encodings, encoded surrogate values
//! (U+D800..U+DFFF), scalar values above U+10FFFF, isolated continuation bytes, and
//! truncated multibyte sequences. Pure ASCII and the empty buffer are valid.
[[nodiscard]] bool is_valid_utf8(std::span<const std::byte> bytes) noexcept;

//! True if the text contains an embedded NUL (U+0000).
[[nodiscard]] bool contains_nul(std::string_view text) noexcept;

//! Validating UTF-16 -> UTF-8 conversion. Rejects odd-length input, isolated high or low
//! surrogates, and truncated surrogate pairs. Never substitutes U+FFFD or '?'.
//! @param big_endian interpret each code unit as big-endian when true, little-endian otherwise.
//! @param out receives the UTF-8 result only on success; cleared and rebuilt each call.
//! @returns true on success, false if the input is not valid UTF-16.
[[nodiscard]] bool utf16_to_utf8(std::span<const std::byte> bytes, bool big_endian, std::string& out);

} // namespace foo_cue_charset::encoding
