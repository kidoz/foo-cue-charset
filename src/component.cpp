#include <SDK/foobar2000-lite.h>
#include <SDK/componentversion.h>

// FOO_CUE_CHARSET_VERSION is injected by meson.build from the project version.
#ifndef FOO_CUE_CHARSET_VERSION
#define FOO_CUE_CHARSET_VERSION "0.0.0-unversioned"
#endif

// The about text states the supported behavior precisely: Unicode detection plus a configured
// legacy fallback. It deliberately does not claim automatic language/charset detection.
// SDK registration macros expand to global factories/classes whose shape we cannot change.
// NOLINTBEGIN
DECLARE_COMPONENT_VERSION("CUE Charset", FOO_CUE_CHARSET_VERSION,
                          "Reads legacy-encoded external CUE sheets by converting their text and referenced "
                          "filenames to UTF-8 in memory.\n"
                          "\n"
                          "Automatic mode detects a BOM or valid UTF-8 and otherwise decodes with a configured "
                          "legacy fallback (Windows-1251, KOI8-R, CP866 or ISO-8859-5). It does not perform "
                          "statistical language detection. Reading is transparent and never modifies the original "
                          "CUE file.\n"
                          "\n"
                          "Registered as a redirecting input for the .cue extension; audio decoding is delegated to "
                          "the normal installed decoders.");

VALIDATE_COMPONENT_FILENAME("foo_cue_charset.dll");
// NOLINTEND
