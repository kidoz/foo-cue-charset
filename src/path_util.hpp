#pragma once

// SDK-independent, Windows-independent path helpers used by the CUE FILE resolver. Kept pure so
// the extension-fallback logic can be unit-tested without the foobar2000 SDK or a real filesystem.

#include <string>
#include <string_view>

namespace foo_cue_charset::path_util {

//! True if the reference looks absolute (drive letter, UNC, leading separator, or a URL scheme)
//! rather than relative to the CUE file's directory.
[[nodiscard]] bool is_absolute_reference(std::string_view path) noexcept;

//! Directory portion of a path including the trailing separator, or empty if the path has no
//! separator (e.g. a bare filename).
[[nodiscard]] std::string directory_prefix(std::string_view path);

//! Replaces the extension of the final path component with new_extension (which must include the
//! leading dot). Returns an empty string if the final component has no extension, so callers can
//! tell "no replacement possible" from a real result.
[[nodiscard]] std::string replace_extension(std::string_view path, std::string_view new_extension);

} // namespace foo_cue_charset::path_util
