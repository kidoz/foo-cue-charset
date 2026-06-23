// Service registration for the CUE Charset input, isolated in its own translation unit.
//
// Instantiating the SDK's input_factory_t / input_entry_impl_t template forces instantiation of
// SDK template methods that emit /W4 C4100 (unreferenced formal parameter) warnings in NDEBUG
// (Release) builds. Compiling this single file at the relaxed SDK warning level (see meson.build)
// keeps those third-party-template warnings out of the component's /W4 /WX net without
// suppressing any warnings in component-owned code (cue_input.cpp etc.).

#include "cue_input.hpp"

namespace foo_cue_charset {
namespace {

// Registered as a redirecting input (input_entry::flag_redirect): it opens another input for the
// referenced audio, so the core knows not to re-enter our handler — there is no recursion.
input_factory_t<cue_charset_input, input_entry::flag_redirect> g_cue_charset_input_factory;

} // namespace
} // namespace foo_cue_charset
