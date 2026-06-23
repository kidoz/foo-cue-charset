#include "encoding/utf.hpp"

#include <cstdint>

namespace foo_cue_charset::encoding {

namespace {

constexpr std::uint32_t kSurrogateHighStart = 0xD800;
constexpr std::uint32_t kSurrogateHighEnd = 0xDBFF;
constexpr std::uint32_t kSurrogateLowStart = 0xDC00;
constexpr std::uint32_t kSurrogateLowEnd = 0xDFFF;
constexpr std::uint32_t kMaxScalar = 0x10FFFF;
constexpr std::uint32_t kSupplementaryBase = 0x10000;

[[nodiscard]] constexpr std::uint8_t to_u8(std::byte b) noexcept {
  return static_cast<std::uint8_t>(b);
}

//! Appends one Unicode scalar value to a UTF-8 string. Caller guarantees scalar is valid
//! (<= U+10FFFF and not a surrogate).
void append_utf8(std::string& out, std::uint32_t scalar) {
  if (scalar <= 0x7F) {
    out.push_back(static_cast<char>(scalar));
  } else if (scalar <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (scalar >> 6)));
    out.push_back(static_cast<char>(0x80 | (scalar & 0x3F)));
  } else if (scalar <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | (scalar >> 12)));
    out.push_back(static_cast<char>(0x80 | ((scalar >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (scalar & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (scalar >> 18)));
    out.push_back(static_cast<char>(0x80 | ((scalar >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((scalar >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (scalar & 0x3F)));
  }
}

} // namespace

bom_info detect_bom(std::span<const std::byte> bytes) noexcept {
  if (bytes.size() >= 3 && to_u8(bytes[0]) == 0xEF && to_u8(bytes[1]) == 0xBB && to_u8(bytes[2]) == 0xBF) {
    return {bom_type::utf8, 3};
  }
  if (bytes.size() >= 2 && to_u8(bytes[0]) == 0xFF && to_u8(bytes[1]) == 0xFE) {
    return {bom_type::utf16_le, 2};
  }
  if (bytes.size() >= 2 && to_u8(bytes[0]) == 0xFE && to_u8(bytes[1]) == 0xFF) {
    return {bom_type::utf16_be, 2};
  }
  return {bom_type::none, 0};
}

bool is_valid_utf8(std::span<const std::byte> bytes) noexcept {
  std::size_t i = 0;
  const std::size_t n = bytes.size();
  while (i < n) {
    const std::uint8_t lead = to_u8(bytes[i]);
    std::size_t extra = 0;
    std::uint32_t scalar = 0;
    std::uint32_t lower_bound = 0; // smallest value legally encodable with this length

    if (lead < 0x80) {
      ++i;
      continue;
    }
    if ((lead & 0xE0) == 0xC0) {
      extra = 1;
      scalar = lead & 0x1F;
      lower_bound = 0x80;
    } else if ((lead & 0xF0) == 0xE0) {
      extra = 2;
      scalar = lead & 0x0F;
      lower_bound = 0x800;
    } else if ((lead & 0xF8) == 0xF0) {
      extra = 3;
      scalar = lead & 0x07;
      lower_bound = 0x10000;
    } else {
      return false; // isolated continuation byte (0x80..0xBF) or invalid lead (0xF8..0xFF)
    }

    if (i + extra >= n) {
      return false; // truncated multibyte sequence (not enough continuation bytes)
    }
    for (std::size_t k = 1; k <= extra; ++k) {
      const std::uint8_t cont = to_u8(bytes[i + k]);
      if ((cont & 0xC0) != 0x80) {
        return false; // missing continuation byte
      }
      scalar = (scalar << 6) | (cont & 0x3F);
    }

    if (scalar < lower_bound) {
      return false; // overlong encoding
    }
    if (scalar >= kSurrogateHighStart && scalar <= kSurrogateLowEnd) {
      return false; // encoded UTF-16 surrogate
    }
    if (scalar > kMaxScalar) {
      return false; // beyond Unicode range
    }
    i += extra + 1;
  }
  return true;
}

bool contains_nul(std::string_view text) noexcept {
  return text.find('\0') != std::string_view::npos;
}

bool utf16_to_utf8(std::span<const std::byte> bytes, bool big_endian, std::string& out) {
  out.clear();
  if (bytes.size() % 2 != 0) {
    return false; // odd-length UTF-16 input
  }
  const std::size_t units = bytes.size() / 2;
  out.reserve(bytes.size());

  auto read_unit = [&](std::size_t unit_index) -> std::uint32_t {
    const std::uint8_t b0 = to_u8(bytes[unit_index * 2]);
    const std::uint8_t b1 = to_u8(bytes[(unit_index * 2) + 1]);
    return big_endian ? ((static_cast<std::uint32_t>(b0) << 8) | b1) : ((static_cast<std::uint32_t>(b1) << 8) | b0);
  };

  for (std::size_t i = 0; i < units; ++i) {
    std::uint32_t unit = read_unit(i);
    if (unit >= kSurrogateHighStart && unit <= kSurrogateHighEnd) {
      if (i + 1 >= units) {
        return false; // truncated surrogate pair
      }
      const std::uint32_t low = read_unit(i + 1);
      if (low < kSurrogateLowStart || low > kSurrogateLowEnd) {
        return false; // high surrogate not followed by a low surrogate
      }
      const std::uint32_t scalar =
          kSupplementaryBase + ((unit - kSurrogateHighStart) << 10) + (low - kSurrogateLowStart);
      append_utf8(out, scalar);
      ++i; // consumed the low surrogate
    } else if (unit >= kSurrogateLowStart && unit <= kSurrogateLowEnd) {
      return false; // isolated low surrogate
    } else {
      append_utf8(out, unit);
    }
  }
  return true;
}

} // namespace foo_cue_charset::encoding
