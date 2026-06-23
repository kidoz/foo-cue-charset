#pragma once

// Preferences page registration for the CUE Charset component. The page lives under the
// Tools branch and edits the persisted configuration in config.hpp.

#include <SDK/foobar2000-lite.h>

namespace foo_cue_charset::preferences {

// Stable GUID of the preferences page; also reported by the input as its preferences page.
// {1177CA81-E67A-41A6-8EAB-7A232C034111}
extern const GUID guid_preferences_page;

} // namespace foo_cue_charset::preferences
