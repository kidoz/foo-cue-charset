#pragma once

// Configuration accessors for the CUE Charset component. Values are persisted through the
// foobar2000 configStore (cfg_var_modern, thread-safe) and validated on read so corrupt or
// future-version data falls back to safe defaults. This header stays free of <windows.h>.

#include "encoding/decoder.hpp"

namespace foo_cue_charset::config {

// Persisted detection-mode values.
inline constexpr int mode_automatic = 0;
inline constexpr int mode_force_selected = 1;

// Persisted legacy-encoding dropdown indices (also the combo-box item order).
inline constexpr int legacy_windows_1251 = 0;
inline constexpr int legacy_koi8_r = 1;
inline constexpr int legacy_cp866 = 2;
inline constexpr int legacy_iso_8859_5 = 3;
inline constexpr int legacy_choice_count = 4;

inline constexpr int default_mode = mode_automatic;
inline constexpr int default_legacy_index = legacy_windows_1251;
inline constexpr bool default_logging = false;

// Raw persisted accessors used by the preferences page.
[[nodiscard]] int get_mode();
void set_mode(int mode);
[[nodiscard]] int get_legacy_index();
void set_legacy_index(int index);
[[nodiscard]] bool get_logging();
void set_logging(bool enabled);

// Maps a (possibly out-of-range) dropdown index to a legacy text_encoding, clamping to the
// Windows-1251 default for unknown values.
[[nodiscard]] encoding::text_encoding legacy_from_index(int index);

// Builds decode options from the current persisted configuration, validated.
[[nodiscard]] encoding::decode_options current_options();

// Whether to log encoding decisions to the foobar2000 Console.
[[nodiscard]] bool logging_enabled();

} // namespace foo_cue_charset::config
