#include "encoding/decoder.hpp"

#include "encoding/utf.hpp"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <optional>

// Win32 code-page conversion lives here only; the public headers stay Windows-free.
#include <windows.h>

namespace foo_cue_charset::encoding {

namespace {

// Windows NLS code-page identifiers for the supported legacy encodings. Named constants
// rather than magic numbers scattered across the code base.
constexpr UINT kCodePageWindows1251 = 1251;
constexpr UINT kCodePageKoi8R = 20866;
constexpr UINT kCodePageCp866 = 866;
constexpr UINT kCodePageIso8859_5 = 28595;

[[nodiscard]] std::optional<UINT> code_page_for(text_encoding enc) noexcept {
  switch (enc) {
  case text_encoding::windows_1251:
    return kCodePageWindows1251;
  case text_encoding::koi8_r:
    return kCodePageKoi8R;
  case text_encoding::cp866:
    return kCodePageCp866;
  case text_encoding::iso_8859_5:
    return kCodePageIso8859_5;
  case text_encoding::utf8:
  case text_encoding::utf16_le:
  case text_encoding::utf16_be:
    return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::unexpected<decode_error> make_error(decode_error_code code, std::string message) {
  return std::unexpected<decode_error>(decode_error{code, std::move(message)});
}

[[nodiscard]] std::string as_string(std::span<const std::byte> bytes, const abort_check& check_abort) {
  poll_abort(check_abort);
  std::string out;
  out.reserve(bytes.size());
  while (!bytes.empty()) {
    const auto chunk = bytes.first(std::min(bytes.size(), processing_chunk_bytes));
    out.append(reinterpret_cast<const char*>(chunk.data()), chunk.size());
    bytes = bytes.subspan(chunk.size());
    poll_abort(check_abort);
  }
  return out;
}

//! Converts a bounded chunk from one of the supported single-byte code pages.
[[nodiscard]] std::expected<std::string, decode_error> legacy_chunk_to_utf8(std::span<const std::byte> bytes,
                                                                            UINT code_page) {
  if (bytes.empty()) {
    return std::string{};
  }
  if (bytes.size() > static_cast<std::size_t>(INT_MAX)) {
    return make_error(decode_error_code::input_too_large, "CUE sheet too large to convert");
  }
  // Microsoft's CP1251 mapping leaves 0x98 undefined, but current Windows NLS maps it
  // to U+0098 even with MB_ERR_INVALID_CHARS. The other supported pages define every byte.
  if (code_page == kCodePageWindows1251 && std::ranges::find(bytes, std::byte{0x98}) != bytes.end()) {
    return make_error(decode_error_code::conversion_failed, "undefined byte in Windows-1251 input");
  }
  const int in_len = static_cast<int>(bytes.size());
  const auto* in_ptr = reinterpret_cast<const char*>(bytes.data());

  const int wide_len = MultiByteToWideChar(code_page, MB_ERR_INVALID_CHARS, in_ptr, in_len, nullptr, 0);
  if (wide_len <= 0) {
    return make_error(decode_error_code::conversion_failed,
                      "byte sequence is not valid in the selected legacy encoding");
  }
  std::wstring wide(static_cast<std::size_t>(wide_len), L'\0');
  if (MultiByteToWideChar(code_page, MB_ERR_INVALID_CHARS, in_ptr, in_len, wide.data(), wide_len) != wide_len) {
    return make_error(decode_error_code::conversion_failed, "legacy-to-Unicode conversion failed");
  }

  const int utf8_len =
      WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), wide_len, nullptr, 0, nullptr, nullptr);
  if (utf8_len <= 0) {
    return make_error(decode_error_code::conversion_failed, "Unicode-to-UTF-8 conversion failed");
  }
  std::string out(static_cast<std::size_t>(utf8_len), '\0');
  if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), wide_len, out.data(), utf8_len, nullptr,
                          nullptr) != utf8_len) {
    return make_error(decode_error_code::conversion_failed, "Unicode-to-UTF-8 conversion failed");
  }
  return out;
}

[[nodiscard]] std::expected<std::string, decode_error> legacy_to_utf8(std::span<const std::byte> bytes, UINT code_page,
                                                                      const abort_check& check_abort) {
  poll_abort(check_abort);
  std::string out;
  out.reserve(bytes.size());
  while (!bytes.empty()) {
    const auto chunk = bytes.first(std::min(bytes.size(), processing_chunk_bytes));
    auto converted = legacy_chunk_to_utf8(chunk, code_page);
    if (!converted) {
      return std::unexpected(converted.error());
    }
    out.append(*converted);
    bytes = bytes.subspan(chunk.size());
    poll_abort(check_abort);
  }
  return out;
}

[[nodiscard]] std::expected<decode_result, decode_error> finalize(std::string utf8, text_encoding source, bool had_bom,
                                                                  bool used_legacy_fallback,
                                                                  const abort_check& check_abort) {
  if (contains_nul(utf8, check_abort)) {
    return make_error(decode_error_code::embedded_nul, "CUE sheet contains an embedded NUL character");
  }
  return decode_result{std::move(utf8), source, had_bom, used_legacy_fallback};
}

//! Decode the whole buffer as a specific encoding (no detection). Used by force mode and by
//! the automatic legacy fallback. A leading BOM matching the target Unicode form is removed.
[[nodiscard]] std::expected<decode_result, decode_error> decode_as(std::span<const std::byte> input, text_encoding enc,
                                                                   bool used_legacy_fallback,
                                                                   const abort_check& check_abort) {
  switch (enc) {
  case text_encoding::utf8: {
    auto body = input;
    bool had_bom = false;
    if (const auto bom = detect_bom(input); bom.type == bom_type::utf8) {
      body = input.subspan(bom.length);
      had_bom = true;
    }
    if (!is_valid_utf8(body, check_abort)) {
      return make_error(decode_error_code::invalid_utf8, "input is not valid UTF-8");
    }
    return finalize(as_string(body, check_abort), text_encoding::utf8, had_bom, used_legacy_fallback, check_abort);
  }
  case text_encoding::utf16_le:
  case text_encoding::utf16_be: {
    const bool big_endian = enc == text_encoding::utf16_be;
    auto body = input;
    bool had_bom = false;
    if (const auto bom = detect_bom(input);
        (big_endian && bom.type == bom_type::utf16_be) || (!big_endian && bom.type == bom_type::utf16_le)) {
      body = input.subspan(bom.length);
      had_bom = true;
    }
    std::string utf8;
    if (!utf16_to_utf8(body, big_endian, utf8, check_abort)) {
      return make_error(decode_error_code::invalid_utf16, "input is not valid UTF-16");
    }
    return finalize(std::move(utf8), enc, had_bom, used_legacy_fallback, check_abort);
  }
  case text_encoding::windows_1251:
  case text_encoding::koi8_r:
  case text_encoding::cp866:
  case text_encoding::iso_8859_5: {
    const auto code_page = code_page_for(enc);
    if (!code_page) {
      return make_error(decode_error_code::unsupported_encoding, "unsupported legacy encoding");
    }
    auto converted = legacy_to_utf8(input, *code_page, check_abort);
    if (!converted) {
      return std::unexpected(converted.error());
    }
    return finalize(std::move(*converted), enc, false, used_legacy_fallback, check_abort);
  }
  }
  return make_error(decode_error_code::unsupported_encoding, "unsupported encoding");
}

} // namespace

std::expected<decode_result, decode_error> decode(std::span<const std::byte> input, const decode_options& options,
                                                  const abort_check& check_abort) {
  poll_abort(check_abort);
  // Enforce the size cap before any allocation or processing.
  if (input.size() > options.maximum_input_bytes) {
    return make_error(decode_error_code::input_too_large, "external CUE exceeds the maximum allowed size");
  }

  if (options.mode == detection_mode::force_selected) {
    return decode_as(input, options.selected_legacy_encoding, /*used_legacy_fallback=*/false, check_abort);
  }

  // Automatic mode: BOM first.
  const auto bom = detect_bom(input);
  if (bom.type == bom_type::utf8) {
    auto body = input.subspan(bom.length);
    if (!is_valid_utf8(body, check_abort)) {
      return make_error(decode_error_code::invalid_utf8, "UTF-8 BOM present but the body is not valid UTF-8");
    }
    return finalize(as_string(body, check_abort), text_encoding::utf8, /*had_bom=*/true,
                    /*used_legacy_fallback=*/false, check_abort);
  }
  if (bom.type == bom_type::utf16_le || bom.type == bom_type::utf16_be) {
    const bool big_endian = bom.type == bom_type::utf16_be;
    const auto enc = big_endian ? text_encoding::utf16_be : text_encoding::utf16_le;
    std::string utf8;
    if (!utf16_to_utf8(input.subspan(bom.length), big_endian, utf8, check_abort)) {
      return make_error(decode_error_code::invalid_utf16, "UTF-16 BOM present but the body is not valid UTF-16");
    }
    return finalize(std::move(utf8), enc, /*had_bom=*/true, /*used_legacy_fallback=*/false, check_abort);
  }

  // No BOM: strict UTF-8 (pure ASCII validates as UTF-8) ...
  if (is_valid_utf8(input, check_abort)) {
    return finalize(as_string(input, check_abort), text_encoding::utf8, /*had_bom=*/false,
                    /*used_legacy_fallback=*/false, check_abort);
  }

  // ... otherwise fall back to the selected legacy encoding.
  return decode_as(input, options.selected_legacy_encoding, /*used_legacy_fallback=*/true, check_abort);
}

const char* display_name(text_encoding enc) noexcept {
  switch (enc) {
  case text_encoding::utf8:
    return "UTF-8";
  case text_encoding::utf16_le:
    return "UTF-16 LE";
  case text_encoding::utf16_be:
    return "UTF-16 BE";
  case text_encoding::windows_1251:
    return "Windows-1251";
  case text_encoding::koi8_r:
    return "KOI8-R";
  case text_encoding::cp866:
    return "CP866";
  case text_encoding::iso_8859_5:
    return "ISO-8859-5";
  }
  return "unknown";
}

bool is_legacy_encoding(text_encoding enc) noexcept {
  return code_page_for(enc).has_value();
}

} // namespace foo_cue_charset::encoding
