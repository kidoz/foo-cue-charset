#pragma once

// Redirecting input that handles EXTERNAL .cue sheets. It reads the raw .cue bytes, converts
// them to UTF-8 with the SDK-independent encoding module, parses them with the official
// cue_parser, exposes one subsong per CUE track, and delegates audio decoding of each track's
// [start, length] segment to the normal installed decoders via the official input_helper_cue.
//
// Registered with input_entry::flag_redirect (see cue_input.cpp): it opens another input for
// the referenced audio, so the core knows not to re-enter it and there is no recursion.

#include <SDK/foobar2000-lite.h>
#include <SDK/input_impl.h>
#include <SDK/playable_location.h>
#include <helpers/cue_parser.h>
#include <helpers/input_helper_cue.h>
#include <helpers/input_helpers.h>

#include <string>
#include <vector>

namespace foo_cue_charset {

class cue_charset_input : public input_stubs {
 public:
  // input_impl (multi-subsong) surface.
  void open(service_ptr_t<file> p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort);

  unsigned get_subsong_count();
  t_uint32 get_subsong(unsigned p_index);
  void get_info(t_uint32 p_subsong, file_info& p_info, abort_callback& p_abort);
  t_filestats2 get_stats2(uint32_t p_flags, abort_callback& p_abort);

  void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback& p_abort);
  bool decode_run(audio_chunk& p_chunk, abort_callback& p_abort);
  void decode_seek(double p_seconds, abort_callback& p_abort);
  bool decode_can_seek();

  // Read-only contract: tag writing is unsupported. open() throws exception_tagging_unsupported
  // for input_open_info_write, so these are never actually called; they exist for the factory
  // template to compile.
  void retag_set_info(t_uint32 p_subsong, const file_info& p_info, abort_callback& p_abort);
  void retag_commit(abort_callback& p_abort);
  void remove_tags(abort_callback& p_abort);

  // input_entry surface.
  static bool g_is_our_content_type(const char* p_content_type);
  static bool g_is_our_path(const char* p_path, const char* p_extension);
  static GUID g_get_guid();
  static const char* g_get_name();
  static GUID g_get_preferences_guid();

 private:
  // A distinct referenced audio file (FILE directive target), resolved and probed once.
  struct audio_source {
    pfc::string8 path;      // resolved, canonical
    file_info_impl info;    // technical info of the audio file (length, samplerate, ...)
    double length = -1.0;   // total audio duration, <0 if unknown
    bool reachable = false; // false if the referenced file could not be opened for info
    bool binary = false;    // raw CD PCM, decoded by the SDK binary reader
  };

  struct track_entry {
    unsigned number = 0;         // CUE TRACK number, also the subsong id
    size_t source = 0;           // index into m_sources
    double start = 0.0;          // playback start (INDEX 01) in seconds
    double decode_length = -1.0; // bounded segment length; <0 means "decode to end of audio"
  };

  size_t intern_source(const char* raw_file, const char* cue_path, bool binary, abort_callback& abort);
  const track_entry& track_for_subsong(t_uint32 subsong) const;
  void build_tracks(const char* cue_path, abort_callback& abort);

  pfc::string8 m_path;
  t_filestats2 m_stats = filestats2_invalid;
  pfc::string8 m_cuesheet_utf8; // converted, BOM-free; consumed by cue_parser::parse_info
  std::vector<audio_source> m_sources;
  std::vector<track_entry> m_tracks;

  input_helper_cue m_decoder;
  bool m_decoding = false;
};

} // namespace foo_cue_charset
