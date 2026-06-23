#include "config.hpp"

#include <SDK/foobar2000-lite.h>
#include <SDK/cfg_var.h>

namespace foo_cue_charset::config {

namespace {

// Stable project-specific configuration GUIDs, generated once for foo_cue_charset.
// {450C77D3-C43E-4FED-AEB0-5DD8F0DD6D9F}
constexpr GUID guid_cfg_mode = {0x450c77d3, 0xc43e, 0x4fed, {0xae, 0xb0, 0x5d, 0xd8, 0xf0, 0xdd, 0x6d, 0x9f}};
// {F73DAD29-1DA0-4B1C-AA64-3E253FD2E21A}
constexpr GUID guid_cfg_encoding = {0xf73dad29, 0x1da0, 0x4b1c, {0xaa, 0x64, 0x3e, 0x25, 0x3f, 0xd2, 0xe2, 0x1a}};
// {B4DC3186-5700-4651-A8CD-40F2D1AC91F4}
constexpr GUID guid_cfg_logging = {0xb4dc3186, 0x5700, 0x4651, {0xa8, 0xcd, 0x40, 0xf2, 0xd1, 0xac, 0x91, 0xf4}};

// SDK-approved global configuration objects. The modern cfg_var classes are internally
// thread-safe (atomic / read-write-locked) and persist through the configStore. Use the
// cfg_var_modern names explicitly so the API does not depend on FOOBAR2000_TARGET_VERSION.
cfg_var_modern::cfg_int g_cfg_mode(guid_cfg_mode, default_mode);
cfg_var_modern::cfg_int g_cfg_encoding(guid_cfg_encoding, default_legacy_index);
cfg_var_modern::cfg_bool g_cfg_logging(guid_cfg_logging, default_logging);

} // namespace

int get_mode() {
  const int64_t value = g_cfg_mode.get();
  return (value == mode_force_selected) ? mode_force_selected : mode_automatic;
}

void set_mode(int mode) {
  g_cfg_mode.set(mode == mode_force_selected ? mode_force_selected : mode_automatic);
}

int get_legacy_index() {
  const int64_t value = g_cfg_encoding.get();
  if (value < 0 || value >= legacy_choice_count) {
    return default_legacy_index;
  }
  return static_cast<int>(value);
}

void set_legacy_index(int index) {
  if (index < 0 || index >= legacy_choice_count) {
    index = default_legacy_index;
  }
  g_cfg_encoding.set(index);
}

bool get_logging() {
  return g_cfg_logging.get();
}

void set_logging(bool enabled) {
  g_cfg_logging.set(enabled);
}

encoding::text_encoding legacy_from_index(int index) {
  switch (index) {
  case legacy_koi8_r:
    return encoding::text_encoding::koi8_r;
  case legacy_cp866:
    return encoding::text_encoding::cp866;
  case legacy_iso_8859_5:
    return encoding::text_encoding::iso_8859_5;
  case legacy_windows_1251:
  default:
    return encoding::text_encoding::windows_1251;
  }
}

encoding::decode_options current_options() {
  encoding::decode_options options;
  options.mode = (get_mode() == mode_force_selected) ? encoding::detection_mode::force_selected
                                                     : encoding::detection_mode::automatic;
  options.selected_legacy_encoding = legacy_from_index(get_legacy_index());
  options.maximum_input_bytes = encoding::default_maximum_input_bytes;
  return options;
}

bool logging_enabled() {
  return get_logging();
}

} // namespace foo_cue_charset::config
