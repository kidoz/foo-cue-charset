#include "encoding/decoder.hpp"

#include "fixtures_cyrillic.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <initializer_list>
#include <span>
#include <string>
#include <vector>

namespace enc = foo_cue_charset::encoding;

namespace {

std::vector<std::byte> bytes(std::initializer_list<int> values) {
  std::vector<std::byte> out;
  out.reserve(values.size());
  for (int v : values) {
    out.push_back(static_cast<std::byte>(static_cast<unsigned char>(v)));
  }
  return out;
}

std::span<const std::byte> view(const std::vector<std::byte>& v) {
  return std::span<const std::byte>(v.data(), v.size());
}

// No decode path may introduce U+FFFD (EF BF BD) or a literal '?' substitution.
bool has_replacement_chars(const std::string& s) {
  return s.find("\xEF\xBF\xBD") != std::string::npos;
}

enc::decode_options automatic(enc::text_encoding fallback = enc::text_encoding::windows_1251) {
  return enc::decode_options{enc::detection_mode::automatic, fallback, enc::default_maximum_input_bytes};
}

enc::decode_options forced(enc::text_encoding selected) {
  return enc::decode_options{enc::detection_mode::force_selected, selected, enc::default_maximum_input_bytes};
}

} // namespace

TEST_CASE("Automatic mode keeps pure ASCII as UTF-8", "[decoder][automatic]") {
  const auto input = bytes({'P', 'E', 'R', 'F', 'O', 'R', 'M', 'E', 'R'});
  const auto result = enc::decode(view(input), automatic());
  REQUIRE(result.has_value());
  CHECK(result->source_encoding == enc::text_encoding::utf8);
  CHECK_FALSE(result->had_bom);
  CHECK_FALSE(result->used_legacy_fallback);
  CHECK(result->utf8 == "PERFORMER");
}

TEST_CASE("Automatic mode passes through valid UTF-8 Cyrillic", "[decoder][automatic]") {
  const auto result = enc::decode(fixtures::as_bytes(fixtures::krov_drakona_utf8), automatic());
  REQUIRE(result.has_value());
  CHECK(result->source_encoding == enc::text_encoding::utf8);
  CHECK_FALSE(result->used_legacy_fallback);
  CHECK(result->utf8 == std::string(fixtures::krov_drakona_utf8_text));
}

TEST_CASE("Automatic mode strips a UTF-8 BOM", "[decoder][automatic][bom]") {
  std::vector<std::byte> input = bytes({0xEF, 0xBB, 0xBF});
  for (unsigned char c : fixtures::grifon_utf8) {
    input.push_back(static_cast<std::byte>(c));
  }
  const auto result = enc::decode(view(input), automatic());
  REQUIRE(result.has_value());
  CHECK(result->had_bom);
  CHECK(result->source_encoding == enc::text_encoding::utf8);
  CHECK(result->utf8 == std::string(fixtures::grifon_utf8_text)); // BOM removed
}

TEST_CASE("Automatic mode falls back to Windows-1251 after invalid UTF-8", "[decoder][fallback]") {
  // Windows-1251 bytes for "Кино" are not valid UTF-8, so automatic mode falls back.
  const auto result = enc::decode(fixtures::as_bytes(fixtures::grifon_windows_1251), automatic());
  REQUIRE(result.has_value());
  CHECK(result->used_legacy_fallback);
  CHECK(result->source_encoding == enc::text_encoding::windows_1251);
  CHECK(result->utf8 == std::string(fixtures::grifon_utf8_text));
  CHECK_FALSE(has_replacement_chars(result->utf8));
}

TEST_CASE("Regression: Windows-1251 metal release with guillemets decodes correctly",
          "[decoder][fallback][regression]") {
  // Real-world shape: a Russian heavy-metal release such as
  // "Ария - Серия «Платиновая коллекция» [CD 2].cue" saved as Windows-1251.
  // The guillemets «» (0xAB/0xBB in CP1251) must convert to U+00AB/U+00BB (C2 AB / C2 BB),
  // and the whole line must fall back to Windows-1251 (it is not valid UTF-8).
  SECTION("band name") {
    const auto r = enc::decode(fixtures::as_bytes(fixtures::kraken_windows_1251), automatic());
    REQUIRE(r.has_value());
    CHECK(r->used_legacy_fallback);
    CHECK(r->source_encoding == enc::text_encoding::windows_1251);
    CHECK(r->utf8 == std::string(fixtures::kraken_utf8_text));
    CHECK_FALSE(has_replacement_chars(r->utf8));
  }
  SECTION("album with guillemets and [CD 2]") {
    const auto r = enc::decode(fixtures::as_bytes(fixtures::antologiya_windows_1251), automatic());
    REQUIRE(r.has_value());
    CHECK(r->used_legacy_fallback);
    CHECK(r->utf8 == std::string(fixtures::antologiya_utf8_text));
    CHECK(r->utf8.find("\xC2\xAB") != std::string::npos); // « -> U+00AB
    CHECK(r->utf8.find("\xC2\xBB") != std::string::npos); // » -> U+00BB
    CHECK_FALSE(has_replacement_chars(r->utf8));
  }
}

TEST_CASE("Forced legacy encodings convert exactly", "[decoder][force]") {
  SECTION("Windows-1251") {
    const auto r =
        enc::decode(fixtures::as_bytes(fixtures::ispolnitel_windows_1251), forced(enc::text_encoding::windows_1251));
    REQUIRE(r.has_value());
    CHECK(r->source_encoding == enc::text_encoding::windows_1251);
    CHECK_FALSE(r->used_legacy_fallback);
    CHECK(r->utf8 == std::string(fixtures::ispolnitel_utf8_text));
  }
  SECTION("KOI8-R") {
    const auto r = enc::decode(fixtures::as_bytes(fixtures::krov_drakona_koi8_r), forced(enc::text_encoding::koi8_r));
    REQUIRE(r.has_value());
    CHECK(r->source_encoding == enc::text_encoding::koi8_r);
    CHECK(r->utf8 == std::string(fixtures::krov_drakona_utf8_text));
  }
  SECTION("CP866") {
    const auto r = enc::decode(fixtures::as_bytes(fixtures::albom_cp866), forced(enc::text_encoding::cp866));
    REQUIRE(r.has_value());
    CHECK(r->source_encoding == enc::text_encoding::cp866);
    CHECK(r->utf8 == std::string(fixtures::albom_utf8_text));
  }
  SECTION("ISO-8859-5") {
    const auto r = enc::decode(fixtures::as_bytes(fixtures::pesnya_iso_8859_5), forced(enc::text_encoding::iso_8859_5));
    REQUIRE(r.has_value());
    CHECK(r->source_encoding == enc::text_encoding::iso_8859_5);
    CHECK(r->utf8 == std::string(fixtures::pesnya_utf8_text));
  }
}

TEST_CASE("Force means force: legacy decode ignores accidental UTF-8 validity", "[decoder][force]") {
  // "ABC" is valid UTF-8, but forcing KOI8-R must reinterpret each byte through KOI8-R.
  // All three are ASCII in KOI8-R, so the bytes are unchanged but the source is reported as KOI8-R.
  const auto input = bytes({'A', 'B', 'C'});
  const auto r = enc::decode(view(input), forced(enc::text_encoding::koi8_r));
  REQUIRE(r.has_value());
  CHECK(r->source_encoding == enc::text_encoding::koi8_r);
  CHECK_FALSE(r->used_legacy_fallback);
  CHECK(r->utf8 == "ABC");
}

TEST_CASE("UTF-16 with BOM decodes in automatic mode", "[decoder][utf16]") {
  SECTION("UTF-16 LE") {
    // BOM + "Грифон" (U+0413 U+0440 U+0438 U+0444 U+043E U+043D) as UTF-16LE code units.
    auto input = bytes({0xFF, 0xFE, 0x13, 0x04, 0x40, 0x04, 0x38, 0x04, 0x44, 0x04, 0x3E, 0x04, 0x3D, 0x04});
    const auto r = enc::decode(view(input), automatic());
    REQUIRE(r.has_value());
    CHECK(r->had_bom);
    CHECK(r->source_encoding == enc::text_encoding::utf16_le);
    CHECK(r->utf8 == std::string(fixtures::grifon_utf8_text));
  }
  SECTION("UTF-16 BE") {
    auto input = bytes({0xFE, 0xFF, 0x04, 0x13, 0x04, 0x40, 0x04, 0x38, 0x04, 0x44, 0x04, 0x3E, 0x04, 0x3D});
    const auto r = enc::decode(view(input), automatic());
    REQUIRE(r.has_value());
    CHECK(r->had_bom);
    CHECK(r->source_encoding == enc::text_encoding::utf16_be);
    CHECK(r->utf8 == std::string(fixtures::grifon_utf8_text));
  }
}

TEST_CASE("Oversized input is rejected before allocation", "[decoder][limits]") {
  auto input = bytes({'a', 'b', 'c', 'd', 'e'});
  enc::decode_options opts = automatic();
  opts.maximum_input_bytes = 4;
  const auto r = enc::decode(view(input), opts);
  REQUIRE_FALSE(r.has_value());
  CHECK(r.error().code == enc::decode_error_code::input_too_large);
  CHECK_FALSE(r.error().message.empty());
}

TEST_CASE("Embedded NUL is rejected", "[decoder][nul]") {
  const auto input = bytes({0x41, 0x00, 0x42}); // valid UTF-8, but contains NUL
  const auto r = enc::decode(view(input), automatic());
  REQUIRE_FALSE(r.has_value());
  CHECK(r.error().code == enc::decode_error_code::embedded_nul);
}

TEST_CASE("Invalid UTF-16 fails cleanly without throwing", "[decoder][utf16]") {
  const auto input = bytes({0xFF, 0xFE, 0x3D, 0xD8}); // BOM + isolated high surrogate
  const auto r = enc::decode(view(input), automatic());
  REQUIRE_FALSE(r.has_value());
  CHECK(r.error().code == enc::decode_error_code::invalid_utf16);
}

TEST_CASE("display_name and is_legacy_encoding are correct", "[decoder][meta]") {
  CHECK(std::string(enc::display_name(enc::text_encoding::windows_1251)) == "Windows-1251");
  CHECK(std::string(enc::display_name(enc::text_encoding::koi8_r)) == "KOI8-R");
  CHECK(std::string(enc::display_name(enc::text_encoding::utf8)) == "UTF-8");
  CHECK(enc::is_legacy_encoding(enc::text_encoding::windows_1251));
  CHECK(enc::is_legacy_encoding(enc::text_encoding::iso_8859_5));
  CHECK_FALSE(enc::is_legacy_encoding(enc::text_encoding::utf8));
  CHECK_FALSE(enc::is_legacy_encoding(enc::text_encoding::utf16_le));
}
