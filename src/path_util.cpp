#include "path_util.hpp"

namespace foo_cue_charset::path_util {

namespace {

constexpr std::string_view kSeparators = "\\/";

} // namespace

bool is_absolute_reference(std::string_view path) noexcept {
  if (path.starts_with('/') || path.starts_with('\\')) {
    return true; // UNC or rooted path
  }
  if (path.size() >= 2 && path[1] == ':') {
    return true; // drive-letter path (C:\...)
  }
  return path.find("://") != std::string_view::npos; // URL scheme
}

std::string directory_prefix(std::string_view path) {
  const std::string_view::size_type separator = path.find_last_of(kSeparators);
  if (separator == std::string_view::npos) {
    return {};
  }
  return std::string(path.substr(0, separator + 1));
}

std::string replace_extension(std::string_view path, std::string_view new_extension) {
  const std::string_view::size_type separator = path.find_last_of(kSeparators);
  const std::string_view::size_type dot = path.find_last_of('.');
  // The dot must belong to the final path component (after any separator) to count as an
  // extension; a dot in a parent directory name does not.
  if (dot == std::string_view::npos || (separator != std::string_view::npos && dot < separator)) {
    return {};
  }
  std::string out(path.substr(0, dot));
  out.append(new_extension);
  return out;
}

} // namespace foo_cue_charset::path_util
