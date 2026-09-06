#include "cue_sheet.hpp"
#include "encoding/decoder.hpp"

#include <SDK/file_info_impl.h>
#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace foo_cue_charset;

namespace {

constexpr const char* kCue = "PERFORMER \"Artist\"\nTITLE \"Album\"\nFILE \"image.bin\" BINARY\n"
                             "TRACK 01 AUDIO\nTITLE \"First\"\nINDEX 01 00:00:00\n"
                             "TRACK 02 AUDIO\nINDEX 00 00:28:00\nINDEX 01 00:30:00\n";

// Deterministically emulate cancellation arriving while the SDK's synchronous parser runs.
class abort_on_check : public abort_callback {
 public:
  explicit abort_on_check(unsigned limit) : m_limit(limit) {}
  bool is_aborting() const override { return ++m_checks >= m_limit; }
  abort_callback_event get_abort_event() const override { return nullptr; }

 private:
  unsigned m_limit;
  mutable unsigned m_checks = 0;
};

} // namespace

TEST_CASE("SDK tracks retain binary mode and INDEX 01 boundaries", "[sdk][cue]") {
  abort_callback_dummy abort;
  const auto tracks = parse_cue_sheet(kCue, abort);
  REQUIRE(tracks.size() == 2);
  CHECK(tracks[0].number == 1);
  CHECK(std::string(tracks[0].file) == "image.bin");
  CHECK(tracks[0].binary);
  CHECK(tracks[1].binary);
  CHECK(tracks[0].start == 0.0);
  CHECK(tracks[1].start == 30.0);
  CHECK(checked_track_length(tracks[0].start, tracks[1].start) == 30.0);
}

TEST_CASE("Separate FILE targets may reset timestamps and switch decoding mode", "[sdk][cue]") {
  abort_callback_dummy abort;
  const std::string cue = std::string(kCue) + "FILE \"song.flac\" WAVE\nTRACK 03 AUDIO\nINDEX 01 00:00:00\n"
                                              "FILE \"second.bin\" binary\nTRACK 04 AUDIO\nINDEX 01 00:00:00\n";
  const auto tracks = parse_cue_sheet(cue.c_str(), abort);
  REQUIRE(tracks.size() == 4);
  CHECK_FALSE(tracks[2].binary);
  CHECK(tracks[2].start == 0.0);
  CHECK(tracks[3].binary);
}

TEST_CASE("Equal and decreasing timestamps in one file fail", "[sdk][cue][regression]") {
  abort_callback_dummy abort;
  for (const auto* next : {"01:00:00", "00:30:00"}) {
    const std::string cue = std::string("FILE \"album.wav\" WAVE\nTRACK 01 AUDIO\nINDEX 01 01:00:00\n"
                                        "TRACK 02 AUDIO\nINDEX 01 ") +
                            next + "\n";
    CHECK_THROWS_AS(parse_cue_sheet(cue.c_str(), abort), exception_io_data);
  }
}

TEST_CASE("The same image cannot switch between binary and decoded audio", "[sdk][cue]") {
  abort_callback_dummy abort;
  const std::string cue = std::string(kCue) + "FILE \"image.bin\" WAVE\nTRACK 03 AUDIO\nINDEX 01 01:00:00\n";
  CHECK_THROWS_AS(parse_cue_sheet(cue.c_str(), abort), exception_io_data);
}

TEST_CASE("SDK metadata remains intact", "[sdk][cue]") {
  abort_callback_dummy abort;
  file_info_impl info;
  read_cue_metadata(kCue, 1, info, abort);
  REQUIRE(info.meta_get("title", 0) != nullptr);
  CHECK(std::string(info.meta_get("title", 0)) == "First");
  REQUIRE(info.meta_get("album", 0) != nullptr);
  CHECK(std::string(info.meta_get("album", 0)) == "Album");
}

TEST_CASE("SDK parsing checks cancellation before and after parsing", "[sdk][abort]") {
  for (const unsigned limit : {1U, 2U}) {
    abort_on_check abort(limit);
    CHECK_THROWS_AS(parse_cue_sheet(kCue, abort), exception_aborted);
    abort_on_check metadata_abort(limit);
    file_info_impl info;
    CHECK_THROWS_AS(read_cue_metadata(kCue, 1, info, metadata_abort), exception_aborted);
  }
}

TEST_CASE("Encoding preserves the actual SDK abort exception", "[sdk][abort]") {
  abort_on_check abort(4);
  const std::vector<std::byte> input(encoding::processing_chunk_bytes * 8, std::byte{0x41});
  const auto convert = [&] {
    const auto result = encoding::decode(input, {}, [&abort] { abort.check(); });
    REQUIRE(result.has_value());
  };
  CHECK_THROWS_AS(convert(), exception_aborted);
}
