#include "encoding/utf.hpp"

#include <cstdint>

namespace foo_cue_charset::encoding {

namespace {

constexpr std::uint32_t kSurrogateHighStart = 0xD800U;
constexpr std::uint32_t kSurrogateHighEnd = 0xDBFFU;
constexpr std::uint32_t kSurrogateLowStart = 0xDC00U;
constexpr std::uint32_t kSurrogateLowEnd = 0xDFFFU;
constexpr std::uint32_t kMaxScalar = 0x10FFFFU;
constexpr std::uint32_t kSupplementaryBase = 0x10000U;

[[nodiscard]] constexpr std::uint32_t byte_value(std::byte b) noexcept {
  return static_cast<std::uint32_t>(b);
}

//! Appends one Unicode scalar value to a UTF-8 string. Caller guarantees scalar is valid
//! (<= U+10FFFF and not a surrogate).
void append_utf8(std::string& out, std::uint32_t scalar) {
  if (scalar <= 0x7FU) {
    out.push_back(static_cast<char>(scalar));
  } else if (scalar <= 0x7FFU) {
    out.push_back(static_cast<char>(0xC0U | (scalar >> 6U)));
    out.push_back(static_cast<char>(0x80U | (scalar & 0x3FU)));
  } else if (scalar <= 0xFFFFU) {
    out.push_back(static_cast<char>(0xE0U | (scalar >> 12U)));
    out.push_back(static_cast<char>(0x80U | ((scalar >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (scalar & 0x3FU)));
  } else {
    out.push_back(static_cast<char>(0xF0U | (scalar >> 18U)));
    out.push_back(static_cast<char>(0x80U | ((scalar >> 12U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | ((scalar >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (scalar & 0x3FU)));
  }
}

} // namespace

bom_info detect_bom(std::span<const std::byte> bytes) noexcept {
  if (bytes.size() >= 3 && byte_value(bytes[0]) == 0xEFU && byte_value(bytes[1]) == 0xBBU &&
      byte_value(bytes[2]) == 0xBFU) {
    return {bom_type::utf8, 3};
  }
  if (bytes.size() >= 2 && byte_value(bytes[0]) == 0xFFU && byte_value(bytes[1]) == 0xFEU) {
    return {bom_type::utf16_le, 2};
  }
  if (bytes.size() >= 2 && byte_value(bytes[0]) == 0xFEU && byte_value(bytes[1]) == 0xFFU) {
    return {bom_type::utf16_be, 2};
  }
  return {bom_type::none, 0};
}

bool is_valid_utf8(std::span<const std::byte> bytes, const abort_check& check_abort) {
  poll_abort(check_abort);
  std::size_t i = 0;
  std::size_t last_check = 0;
  const std::size_t n = bytes.size();
  while (i < n) {
    if (i - last_check >= processing_chunk_bytes) {
      poll_abort(check_abort);
      last_check = i;
    }
    const std::uint32_t lead = byte_value(bytes[i]);
    std::size_t extra = 0;
    std::uint32_t scalar = 0;
    std::uint32_t lower_bound = 0; // smallest value legally encodable with this length

    if (lead < 0x80U) {
      ++i;
      continue;
    }
    if ((lead & 0xE0U) == 0xC0U) {
      extra = 1;
      scalar = lead & 0x1FU;
      lower_bound = 0x80U;
    } else if ((lead & 0xF0U) == 0xE0U) {
      extra = 2;
      scalar = lead & 0x0FU;
      lower_bound = 0x800U;
    } else if ((lead & 0xF8U) == 0xF0U) {
      extra = 3;
      scalar = lead & 0x07U;
      lower_bound = 0x10000U;
    } else {
      return false; // isolated continuation byte (0x80..0xBF) or invalid lead (0xF8..0xFF)
    }

    if (i + extra >= n) {
      return false; // truncated multibyte sequence (not enough continuation bytes)
    }
    for (std::size_t k = 1; k <= extra; ++k) {
      const std::uint32_t cont = byte_value(bytes[i + k]);
      if ((cont & 0xC0U) != 0x80U) {
        return false; // missing continuation byte
      }
      scalar = (scalar << 6U) | (cont & 0x3FU);
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
  poll_abort(check_abort);
  return true;
}

bool contains_nul(std::string_view text, const abort_check& check_abort) {
  poll_abort(check_abort);
  while (!text.empty()) {
    const auto chunk = text.substr(0, processing_chunk_bytes);
    if (chunk.find('\0') != std::string_view::npos) {
      return true;
    }
    text.remove_prefix(chunk.size());
    poll_abort(check_abort);
  }
  return false;
}

bool utf16_to_utf8(std::span<const std::byte> bytes, bool big_endian, std::string& out,
                   const abort_check& check_abort) {
  poll_abort(check_abort);
  out.clear();
  if (bytes.size() % 2 != 0) {
    return false; // odd-length UTF-16 input
  }
  const std::size_t units = bytes.size() / 2;
  out.reserve(bytes.size());

  auto read_unit = [&](std::size_t unit_index) -> std::uint32_t {
    const std::uint32_t b0 = byte_value(bytes[unit_index * 2]);
    const std::uint32_t b1 = byte_value(bytes[(unit_index * 2) + 1]);
    return big_endian ? ((static_cast<std::uint32_t>(b0) << 8U) | b1) : ((static_cast<std::uint32_t>(b1) << 8U) | b0);
  };

  std::size_t last_check = 0;
  for (std::size_t i = 0; i < units; ++i) {
    if (i - last_check >= processing_chunk_bytes / 2) {
      poll_abort(check_abort);
      last_check = i;
    }
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
          kSupplementaryBase + ((unit - kSurrogateHighStart) << 10U) + (low - kSurrogateLowStart);
      append_utf8(out, scalar);
      ++i; // consumed the low surrogate
    } else if (unit >= kSurrogateLowStart && unit <= kSurrogateLowEnd) {
      return false; // isolated low surrogate
    } else {
      append_utf8(out, unit);
    }
  }
  poll_abort(check_abort);
  return true;
}

} // namespace foo_cue_charset::encoding
