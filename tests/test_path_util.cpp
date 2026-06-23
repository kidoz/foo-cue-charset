#include "path_util.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace pu = foo_cue_charset::path_util;

TEST_CASE("is_absolute_reference distinguishes absolute from relative", "[path]") {
  CHECK(pu::is_absolute_reference("C:\\music\\x.flac"));
  CHECK(pu::is_absolute_reference("\\\\server\\share\\x.flac"));
  CHECK(pu::is_absolute_reference("/abs/x.flac"));
  CHECK(pu::is_absolute_reference("\\rooted\\x.flac"));
  CHECK(pu::is_absolute_reference("file://host/x.flac"));
  CHECK_FALSE(pu::is_absolute_reference("x.flac"));
  CHECK_FALSE(pu::is_absolute_reference("sub/dir/x.flac"));
  CHECK_FALSE(pu::is_absolute_reference("sub\\dir\\x.flac"));
}

TEST_CASE("directory_prefix returns the directory with trailing separator", "[path]") {
  CHECK(pu::directory_prefix("a/b/c.wav") == "a/b/");
  CHECK(pu::directory_prefix("a\\b\\c.wav") == "a\\b\\");
  CHECK(pu::directory_prefix("c.wav").empty());
  CHECK(pu::directory_prefix("C:\\album\\cd.cue") == "C:\\album\\");
}

TEST_CASE("replace_extension swaps only the final component's extension", "[path]") {
  CHECK(pu::replace_extension("Song.wav", ".flac") == "Song.flac");
  CHECK(pu::replace_extension("dir/Song.wav", ".flac") == "dir/Song.flac");
  CHECK(pu::replace_extension("C:\\a\\b\\Song.ape", ".flac") == "C:\\a\\b\\Song.flac");
  // Real-world shape: name with spaces, guillemets and [CD 2].
  CHECK(pu::replace_extension("Band - Series «X» [CD 2].wav", ".flac") == "Band - Series «X» [CD 2].flac");
}

TEST_CASE("replace_extension returns empty when the final component has no extension", "[path]") {
  CHECK(pu::replace_extension("noext", ".flac").empty());
  // A dot only in a parent directory name is not an extension of the final component.
  CHECK(pu::replace_extension("dir.with.dot/file", ".flac").empty());
  CHECK(pu::replace_extension("a/b.c/d", ".flac").empty());
}
