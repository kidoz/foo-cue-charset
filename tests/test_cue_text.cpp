#include "encoding/decoder.hpp"

#include "fixtures_cyrillic.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace enc = foo_cue_charset::encoding;

namespace {

// Fictional example release used throughout the tests: the band Химера, album "Песни сирен",
// referenced file "Химера - Песни сирен.flac", plus a second track "Гром".
enum class phrase { himera, pesni_siren, grom, file_himera };

std::span<const unsigned char> legacy_w1251(phrase p) {
  switch (p) {
  case phrase::himera:
    return fixtures::himera_windows_1251;
  case phrase::pesni_siren:
    return fixtures::pesni_siren_windows_1251;
  case phrase::grom:
    return fixtures::grom_windows_1251;
  case phrase::file_himera:
    return fixtures::file_himera_windows_1251;
  }
  return {};
}

std::string_view expected_utf8(phrase p) {
  switch (p) {
  case phrase::himera:
    return fixtures::himera_utf8_text;
  case phrase::pesni_siren:
    return fixtures::pesni_siren_utf8_text;
  case phrase::grom:
    return fixtures::grom_utf8_text;
  case phrase::file_himera:
    return fixtures::file_himera_utf8_text;
  }
  return {};
}

// Builds the same CUE document either as Windows-1251 bytes (legacy=true) or as the
// canonical UTF-8 string (legacy=false), with the given line ending.
template <bool Legacy> std::string build_cue(std::string_view eol) {
  std::string out;
  auto lit = [&](std::string_view s) { out.append(s); };
  auto ph = [&](phrase p) {
    if constexpr (Legacy) {
      for (unsigned char b : legacy_w1251(p)) {
        out.push_back(static_cast<char>(b));
      }
    } else {
      out.append(expected_utf8(p));
    }
  };

  lit("PERFORMER \"");
  ph(phrase::himera);
  lit("\"");
  lit(eol);
  lit("TITLE \"");
  ph(phrase::pesni_siren);
  lit("\"");
  lit(eol);
  lit("REM COMMENT \"");
  ph(phrase::himera);
  lit("\"");
  lit(eol);
  lit("FILE \"");
  ph(phrase::file_himera);
  lit("\" WAVE");
  lit(eol);
  lit("  TRACK 01 AUDIO");
  lit(eol);
  lit("    TITLE \"");
  ph(phrase::pesni_siren);
  lit("\"");
  lit(eol);
  lit("    PERFORMER \"");
  ph(phrase::himera);
  lit("\"");
  lit(eol);
  lit("    INDEX 00 00:00:00");
  lit(eol);
  lit("    INDEX 01 00:00:02");
  lit(eol);
  lit("  TRACK 02 AUDIO");
  lit(eol);
  lit("    TITLE \"");
  ph(phrase::grom);
  lit("\"");
  lit(eol);
  lit("    INDEX 01 03:25:17");
  lit(eol);
  return out;
}

std::span<const std::byte> as_bytes(const std::string& s) {
  return std::span<const std::byte>(reinterpret_cast<const std::byte*>(s.data()), s.size());
}

bool has_replacement_chars(const std::string& s) {
  return s.find("\xEF\xBF\xBD") != std::string::npos;
}

enc::decode_options automatic() {
  return enc::decode_options{enc::detection_mode::automatic, enc::text_encoding::windows_1251,
                             enc::default_maximum_input_bytes};
}

} // namespace

TEST_CASE("Windows-1251 CUE decodes to the exact UTF-8 document (CRLF)", "[cue][automatic]") {
  const std::string legacy = build_cue<true>("\r\n");
  const std::string expected = build_cue<false>("\r\n");

  const auto r = enc::decode(as_bytes(legacy), automatic());
  REQUIRE(r.has_value());
  CHECK(r->used_legacy_fallback);
  CHECK(r->source_encoding == enc::text_encoding::windows_1251);
  CHECK_FALSE(has_replacement_chars(r->utf8));
  CHECK(r->utf8 == expected); // byte-exact, CRLF preserved
}

TEST_CASE("LF line endings are preserved", "[cue][automatic]") {
  const std::string legacy = build_cue<true>("\n");
  const std::string expected = build_cue<false>("\n");

  const auto r = enc::decode(as_bytes(legacy), automatic());
  REQUIRE(r.has_value());
  CHECK(r->utf8 == expected);
  CHECK(r->utf8.find('\r') == std::string::npos); // no CRLF was invented
}

TEST_CASE("Decoded CUE contains the expected album, track and FILE fields", "[cue][fields]") {
  const std::string legacy = build_cue<true>("\r\n");
  const auto r = enc::decode(as_bytes(legacy), automatic());
  REQUIRE(r.has_value());
  const std::string& text = r->utf8;

  // Album-level fields.
  CHECK(text.find(std::string("PERFORMER \"") + std::string(expected_utf8(phrase::himera)) + "\"") !=
        std::string::npos);
  CHECK(text.find(std::string("TITLE \"") + std::string(expected_utf8(phrase::pesni_siren)) + "\"") !=
        std::string::npos);
  // Quoted Cyrillic FILE directive.
  CHECK(text.find(std::string("FILE \"") + std::string(expected_utf8(phrase::file_himera)) + "\" WAVE") !=
        std::string::npos);
  // REM comment field is converted too.
  CHECK(text.find(std::string("REM COMMENT \"") + std::string(expected_utf8(phrase::himera)) + "\"") !=
        std::string::npos);
  // Second track title.
  CHECK(text.find(std::string("TITLE \"") + std::string(expected_utf8(phrase::grom)) + "\"") != std::string::npos);
  // ASCII structure is untouched.
  CHECK(text.find("TRACK 01 AUDIO") != std::string::npos);
  CHECK(text.find("INDEX 01 00:00:02") != std::string::npos);
}

TEST_CASE("A UTF-8 CUE round-trips unchanged in automatic mode", "[cue][automatic]") {
  const std::string utf8doc = build_cue<false>("\r\n");
  const auto r = enc::decode(as_bytes(utf8doc), automatic());
  REQUIRE(r.has_value());
  CHECK(r->source_encoding == enc::text_encoding::utf8);
  CHECK_FALSE(r->used_legacy_fallback);
  CHECK(r->utf8 == utf8doc);
}

TEST_CASE("A malformed forced-UTF-8 CUE fails with a typed error, not an exception", "[cue][malformed]") {
  const std::string legacy = build_cue<true>("\r\n"); // Windows-1251 bytes, invalid as UTF-8
  // Forcing UTF-8 on legacy bytes must return an error value (std::expected), never throw.
  const auto r = enc::decode(as_bytes(legacy), {enc::detection_mode::force_selected, enc::text_encoding::utf8,
                                                enc::default_maximum_input_bytes});
  REQUIRE_FALSE(r.has_value());
  CHECK(r.error().code == enc::decode_error_code::invalid_utf8);
}
