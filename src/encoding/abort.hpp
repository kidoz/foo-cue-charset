#pragma once

#include <cstddef>
#include <functional>

namespace foo_cue_charset::encoding {

//! Optional cancellation callback. Exceptions propagate unchanged, including SDK aborts.
using abort_check = std::function<void()>;
inline constexpr std::size_t processing_chunk_bytes = std::size_t{16} * 1024;

inline void poll_abort(const abort_check& check) {
  if (check) {
    check();
  }
}

} // namespace foo_cue_charset::encoding
