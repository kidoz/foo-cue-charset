#include "encoding/utf.hpp"

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

} // namespace

TEST_CASE("detect_bom identifies each supported BOM", "[utf][bom]") {
  CHECK(enc::detect_bom(view(bytes({0xEF, 0xBB, 0xBF, 0x41}))).type == enc::bom_type::utf8);
  CHECK(enc::detect_bom(view(bytes({0xEF, 0xBB, 0xBF}))).length == 3);
  CHECK(enc::detect_bom(view(bytes({0xFF, 0xFE, 0x41, 0x00}))).type == enc::bom_type::utf16_le);
  CHECK(enc::detect_bom(view(bytes({0xFE, 0xFF, 0x00, 0x41}))).type == enc::bom_type::utf16_be);
  CHECK(enc::detect_bom(view(bytes({0x41, 0x42, 0x43}))).type == enc::bom_type::none);
  CHECK(enc::detect_bom(view(bytes({}))).type == enc::bom_type::none);
  // A bare 0xFF without the second BOM byte is not a UTF-16 BOM.
  CHECK(enc::detect_bom(view(bytes({0xFF}))).type == enc::bom_type::none);
}

TEST_CASE("is_valid_utf8 accepts ASCII, Cyrillic and empty", "[utf][utf8]") {
  CHECK(enc::is_valid_utf8(view(bytes({}))));
  CHECK(enc::is_valid_utf8(view(bytes({0x41, 0x42, 0x43})))); // ABC
  CHECK(enc::is_valid_utf8(fixtures::as_bytes(fixtures::grifon_utf8)));
  CHECK(enc::is_valid_utf8(fixtures::as_bytes(fixtures::krov_drakona_utf8)));
  // 4-byte sequence: U+1F600
  CHECK(enc::is_valid_utf8(view(bytes({0xF0, 0x9F, 0x98, 0x80}))));
}

TEST_CASE("is_valid_utf8 rejects malformed sequences", "[utf][utf8]") {
  SECTION("overlong encoding of NUL") {
    CHECK_FALSE(enc::is_valid_utf8(view(bytes({0xC0, 0x80}))));
  }
  SECTION("overlong 3-byte encoding") {
    CHECK_FALSE(enc::is_valid_utf8(view(bytes({0xE0, 0x80, 0x80}))));
  }
  SECTION("isolated continuation byte") {
    CHECK_FALSE(enc::is_valid_utf8(view(bytes({0x80}))));
    CHECK_FALSE(enc::is_valid_utf8(view(bytes({0x41, 0xBF}))));
  }
  SECTION("truncated multibyte sequence") {
    CHECK_FALSE(enc::is_valid_utf8(view(bytes({0xD0}))));       // lead, no continuation
    CHECK_FALSE(enc::is_valid_utf8(view(bytes({0xE2, 0x82})))); // 2 of 3 bytes
  }
  SECTION("UTF-8 encoded surrogate U+D800") {
    CHECK_FALSE(enc::is_valid_utf8(view(bytes({0xED, 0xA0, 0x80}))));
  }
  SECTION("code point above U+10FFFF") {
    CHECK_FALSE(enc::is_valid_utf8(view(bytes({0xF4, 0x90, 0x80, 0x80})))); // U+110000
    CHECK_FALSE(enc::is_valid_utf8(view(bytes({0xF5, 0x80, 0x80, 0x80})))); // invalid lead
  }
  SECTION("missing continuation in the middle") {
    CHECK_FALSE(enc::is_valid_utf8(view(bytes({0xE2, 0x28, 0xA1}))));
  }
}

TEST_CASE("utf16_to_utf8 converts BMP and surrogate pairs", "[utf][utf16]") {
  std::string out;

  SECTION("ASCII little-endian") {
    REQUIRE(enc::utf16_to_utf8(view(bytes({0x41, 0x00, 0x42, 0x00})), /*big_endian=*/false, out));
    CHECK(out == "AB");
  }
  SECTION("ASCII big-endian") {
    REQUIRE(enc::utf16_to_utf8(view(bytes({0x00, 0x41, 0x00, 0x42})), /*big_endian=*/true, out));
    CHECK(out == "AB");
  }
  SECTION("Cyrillic BMP little-endian (U+0413 U+0440 U+0438 U+0444 U+043E U+043D)") {
    REQUIRE(enc::utf16_to_utf8(view(bytes({0x13, 0x04, 0x40, 0x04, 0x38, 0x04, 0x44, 0x04, 0x3E, 0x04, 0x3D, 0x04})),
                               false, out));
    CHECK(out == std::string(fixtures::grifon_utf8_text));
  }
  SECTION("supplementary plane U+1F600 little-endian") {
    REQUIRE(enc::utf16_to_utf8(view(bytes({0x3D, 0xD8, 0x00, 0xDE})), false, out));
    CHECK(out == std::string("\xF0\x9F\x98\x80"));
  }
  SECTION("supplementary plane U+1F600 big-endian") {
    REQUIRE(enc::utf16_to_utf8(view(bytes({0xD8, 0x3D, 0xDE, 0x00})), true, out));
    CHECK(out == std::string("\xF0\x9F\x98\x80"));
  }
}

TEST_CASE("utf16_to_utf8 rejects malformed UTF-16", "[utf][utf16]") {
  std::string out;
  SECTION("odd length") {
    CHECK_FALSE(enc::utf16_to_utf8(view(bytes({0x41, 0x00, 0x42})), false, out));
  }
  SECTION("isolated high surrogate") {
    CHECK_FALSE(enc::utf16_to_utf8(view(bytes({0x3D, 0xD8})), false, out));
  }
  SECTION("isolated low surrogate") {
    CHECK_FALSE(enc::utf16_to_utf8(view(bytes({0x00, 0xDE})), false, out));
  }
  SECTION("high surrogate followed by non-low") {
    CHECK_FALSE(enc::utf16_to_utf8(view(bytes({0x3D, 0xD8, 0x41, 0x00})), false, out));
  }
}

TEST_CASE("contains_nul detects embedded NUL", "[utf][nul]") {
  CHECK_FALSE(enc::contains_nul("hello"));
  CHECK(enc::contains_nul(std::string_view("a\0b", 3)));
}
