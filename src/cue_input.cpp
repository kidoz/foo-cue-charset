#include "cue_input.hpp"

#include "config.hpp"
#include "encoding/decoder.hpp"
#include "path_util.hpp"
#include "preferences.hpp"

#include <SDK/console.h>
#include <SDK/filesystem.h>

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace foo_cue_charset {

namespace {

// {0CD7B266-0C75-4428-9570-D3151792D3D9}
constexpr GUID guid_input = {0x0cd7b266, 0x0c75, 0x4428, {0x95, 0x70, 0xd3, 0x15, 0x17, 0x92, 0xd3, 0xd9}};

void log_decision(const encoding::decode_result& result) {
  if (!config::logging_enabled()) {
    return;
  }
  pfc::string_formatter msg;
  msg << "foo_cue_charset: decoded external CUE as " << encoding::display_name(result.source_encoding);
  if (result.used_legacy_fallback) {
    msg << " using automatic fallback";
  } else if (result.had_bom) {
    msg << " (BOM detected)";
  }
  console::info(msg.get_ptr());
}

void log_error(const char* message) {
  if (!config::logging_enabled()) {
    return;
  }
  pfc::string_formatter msg;
  msg << "foo_cue_charset: failed to decode external CUE: " << message;
  console::error(msg.get_ptr());
}

void log_reference_extension_fallback(const char* extension) {
  if (!config::logging_enabled()) {
    return;
  }
  pfc::string_formatter msg;
  msg << "foo_cue_charset: resolved missing CUE FILE target by using same-basename " << extension << " file";
  console::info(msg.get_ptr());
}

bool file_exists(const char* path, abort_callback& abort) {
  try {
    return filesystem::g_exists(path, abort);
  } catch (const exception_aborted&) {
    throw;
  } catch (const std::exception&) {
    return false;
  }
}

pfc::string8 canonical_path(const char* path) {
  pfc::string8 out;
  filesystem::g_get_canonical_path(path, out);
  return out;
}

pfc::string8 explicit_cue_relative_path(const char* raw_file, const char* cue_path) {
  if (path_util::is_absolute_reference(raw_file)) {
    return {};
  }
  const std::string directory = path_util::directory_prefix(cue_path);
  if (directory.empty()) {
    return {};
  }
  const std::string combined = directory + raw_file;
  return canonical_path(combined.c_str());
}

// Same-basename audio extensions tried when a CUE FILE target is missing on disk (the common
// "cue says .wav but the audio was transcoded to .flac" case). A fallback is only accepted when
// exactly one candidate exists; multiple candidates are reported as ambiguous rather than guessed.
constexpr const char* kAudioExtensions[] = {".flac", ".wv", ".ape", ".wav", ".mp3", ".m4a", ".mp4", ".aiff", ".aif"};

// Resolves the on-disk target for a reference. Returns the exact path if it exists; otherwise, if
// exactly one same-basename file with a different audio extension exists, returns that (logging the
// substitution). Throws exception_io_data when several same-basename candidates exist, so we never
// silently pick the wrong audio image. Returns empty when nothing matching exists.
pfc::string8 find_existing_reference_variant(const pfc::string8& resolved, abort_callback& abort) {
  if (file_exists(resolved, abort)) {
    return resolved;
  }

  pfc::string8 single_match;
  pfc::string8 matched_extension;
  pfc::string8 candidate_extensions;
  unsigned candidate_count = 0;
  for (const char* extension : kAudioExtensions) {
    const std::string candidate = path_util::replace_extension(resolved.c_str(), extension);
    if (candidate.empty() || pfc::stringEqualsI_ascii(candidate.c_str(), resolved)) {
      continue;
    }
    if (file_exists(candidate.c_str(), abort)) {
      ++candidate_count;
      if (candidate_count == 1) {
        single_match = candidate.c_str();
        matched_extension = extension;
      }
      if (!candidate_extensions.is_empty()) {
        candidate_extensions += ", ";
      }
      candidate_extensions += extension;
    }
  }

  if (candidate_count > 1) {
    pfc::string8 message = "CUE Charset: the referenced audio file is missing and multiple same-named files exist (";
    message += candidate_extensions;
    message += "); edit the FILE line in the CUE to name the correct file";
    throw exception_io_data(message);
  }
  if (candidate_count == 1) {
    log_reference_extension_fallback(matched_extension);
    return single_match;
  }
  return {};
}

// Reads the whole file but never buffers more than maximum_input_bytes (+1 to detect overflow),
// so an oversized or unknown-size CUE is rejected before a large allocation. This is the guarantee;
// open() also does a fast get_size() pre-check when the size is known.
std::vector<std::byte> read_cue_bytes(const service_ptr_t<file>& handle, size_t max_bytes, abort_callback& abort) {
  std::vector<std::byte> out;
  constexpr size_t chunk_size = size_t{16} * 1024;
  std::byte chunk[chunk_size];
  for (;;) {
    abort.check();
    const size_t budget = max_bytes + 1 - out.size(); // read at most max_bytes + 1 total
    const size_t want = std::min(chunk_size, budget);
    if (want == 0) {
      break;
    }
    const size_t got = handle->read(chunk, want, abort);
    if (got == 0) {
      break;
    }
    out.insert(out.end(), chunk, chunk + got);
  }
  if (out.size() > max_bytes) {
    throw exception_io_data("CUE Charset: external CUE exceeds the maximum allowed size");
  }
  return out;
}

pfc::string8 resolve_reference_path(const char* raw_file, const char* cue_path, abort_callback& abort) {
  pfc::string8 resolved;
  if (!filesystem::g_relative_path_parse(raw_file, cue_path, resolved)) {
    resolved = canonical_path(raw_file);
  }

  if (pfc::string8 existing = find_existing_reference_variant(resolved, abort); !existing.is_empty()) {
    return existing;
  }

  if (pfc::string8 explicit_relative = explicit_cue_relative_path(raw_file, cue_path); !explicit_relative.is_empty()) {
    if (pfc::string8 existing = find_existing_reference_variant(explicit_relative, abort); !existing.is_empty()) {
      return existing;
    }
  }

  return resolved;
}

// Copies the technical (non-metadata) fields foobar2000 displays for a track from the
// referenced audio file's info.
void copy_tech_info(file_info& dst, const file_info& src) {
  const char* const int_keys[] = {"samplerate", "channels", "bitspersample", "bitrate"};
  for (const char* key : int_keys) {
    const t_int64 value = src.info_get_int(key);
    if (value > 0) {
      dst.info_set_int(key, value);
    }
  }
  if (const char* codec = src.info_get("codec")) {
    dst.info_set("codec", codec);
  }
  if (const char* encoding_name = src.info_get("encoding")) {
    dst.info_set("encoding", encoding_name);
  }
}

} // namespace

void cue_charset_input::open(service_ptr_t<file> p_filehint, const char* p_path, t_input_open_reason p_reason,
                             abort_callback& p_abort) {
  // Read-only contract: refuse tag writing cleanly.
  if (p_reason == input_open_info_write) {
    throw exception_tagging_unsupported();
  }

  m_path = p_path;

  service_ptr_t<file> f = p_filehint;
  input_open_file_helper(f, p_path, p_reason, p_abort);
  m_stats = f->get_stats2_(stats2_all, p_abort);

  const encoding::decode_options options = config::current_options();

  // Enforce the size cap before allocation. Fast path: reject when the size is known up front.
  const t_filesize size = f->get_size(p_abort);
  if (size != filesize_invalid && size > options.maximum_input_bytes) {
    log_error("external CUE exceeds the maximum allowed size");
    throw exception_io_data("CUE Charset: external CUE exceeds the maximum allowed size");
  }

  // Guarantee: bounded chunked read, so an unknown/invalid reported size cannot read unbounded.
  const std::vector<std::byte> raw = read_cue_bytes(f, options.maximum_input_bytes, p_abort);
  p_abort.check();

  const std::span<const std::byte> bytes(raw.data(), raw.size());
  const auto decoded = encoding::decode(bytes, options);
  if (!decoded) {
    log_error(decoded.error().message.c_str());
    pfc::string8 message = "CUE Charset: ";
    message += decoded.error().message.c_str();
    throw exception_io_data(message);
  }
  log_decision(*decoded);

  m_cuesheet_utf8 = decoded->utf8.c_str();
  build_tracks(p_path, p_abort);

  if (m_tracks.empty()) {
    throw exception_io_data("CUE Charset: the CUE sheet contains no audio tracks");
  }
}

size_t cue_charset_input::intern_source(const char* raw_file, const char* cue_path, abort_callback& abort) {
  const pfc::string8 resolved = resolve_reference_path(raw_file, cue_path, abort);

  for (size_t i = 0; i < m_sources.size(); ++i) {
    if (pfc::stringEqualsI_utf8(m_sources[i].path, resolved)) {
      return i;
    }
  }

  audio_source source;
  source.path = resolved;
  try {
    input_helper::g_get_info(make_playable_location(resolved, 0), source.info, abort, /*p_from_redirect=*/true);
    source.length = source.info.get_length();
    source.reachable = true;
  } catch (const exception_aborted&) {
    throw;
  } catch (const std::exception&) {
    // Missing or unreadable referenced audio: keep the track listed but without duration.
    source.reachable = false;
    source.length = -1.0;
  }

  m_sources.push_back(std::move(source));
  return m_sources.size() - 1;
}

void cue_charset_input::build_tracks(const char* cue_path, abort_callback& abort) {
  m_sources.clear();
  m_tracks.clear();

  cue_parser::t_cue_entry_list entries;
  try {
    cue_parser::parse(m_cuesheet_utf8, entries);
  } catch (const exception_io_data& e) {
    // Surface a clear, sanitized parse failure.
    pfc::string8 message = "CUE Charset: ";
    message += e.what();
    throw exception_io_data(message);
  }

  for (auto iter = entries.first(); iter.is_valid(); ++iter) {
    abort.check();
    const cue_parser::cue_entry& entry = *iter;
    track_entry track;
    track.number = entry.m_track_number;
    track.start = entry.m_indexes.start();
    track.source = intern_source(entry.m_file, cue_path, abort);
    m_tracks.push_back(track);
  }

  // Compute each track's bounded decode length. A track ends where the next track in the same
  // audio file begins; the last track of a file decodes to the end of that file (length < 0).
  for (size_t i = 0; i < m_tracks.size(); ++i) {
    const bool next_same_file = (i + 1 < m_tracks.size()) && (m_tracks[i + 1].source == m_tracks[i].source);
    if (next_same_file) {
      const double length = m_tracks[i + 1].start - m_tracks[i].start;
      m_tracks[i].decode_length = (length > 0.0) ? length : -1.0;
    } else {
      m_tracks[i].decode_length = -1.0;
    }
  }
}

const cue_charset_input::track_entry& cue_charset_input::track_for_subsong(t_uint32 subsong) const {
  for (const track_entry& track : m_tracks) {
    if (track.number == subsong) {
      return track;
    }
  }
  throw exception_io_bad_subsong_index();
}

unsigned cue_charset_input::get_subsong_count() {
  return static_cast<unsigned>(m_tracks.size());
}

t_uint32 cue_charset_input::get_subsong(unsigned p_index) {
  if (p_index >= m_tracks.size()) {
    throw exception_io_bad_subsong_index();
  }
  return m_tracks[p_index].number;
}

void cue_charset_input::get_info(t_uint32 p_subsong, file_info& p_info, abort_callback& p_abort) {
  (void)p_abort;
  const track_entry& track = track_for_subsong(p_subsong);

  p_info.reset();

  // Official parser fills album-level + track-level metadata for this track number.
  cue_parser::parse_info(m_cuesheet_utf8, p_info, track.number);

  // Duration: the bounded segment length, or (for the last track of a file) the remaining
  // duration of the referenced audio.
  double display_length = track.decode_length;
  const audio_source& source = m_sources[track.source];
  if (display_length < 0.0 && source.reachable && source.length > 0.0) {
    display_length = source.length - track.start;
  }
  if (display_length > 0.0) {
    p_info.set_length(display_length);
  }

  if (source.reachable) {
    copy_tech_info(p_info, source.info);
  }
}

t_filestats2 cue_charset_input::get_stats2(uint32_t p_flags, abort_callback& p_abort) {
  (void)p_flags;
  (void)p_abort;
  return m_stats;
}

void cue_charset_input::decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback& p_abort) {
  const track_entry& track = track_for_subsong(p_subsong);
  const audio_source& source = m_sources[track.source];

  unsigned flags = p_flags;
  if (track.start > 0.0) {
    flags &= ~static_cast<unsigned>(input_flag_no_seeking);
  }
  flags &= ~static_cast<unsigned>(input_flag_allow_inaccurate_seeking);

  if (m_decoder.is_open()) {
    m_decoder.close();
  }
  // input_helper_cue treats length > 0 as an exact segment and length <= 0 as "to the end of
  // the referenced audio" (it derives the remainder from the file's own duration). So the
  // last track of a file is passed 0.0, not a negative value.
  const double segment_length = (track.decode_length > 0.0) ? track.decode_length : 0.0;
  m_decoder.open(service_ptr_t<file>(), make_playable_location(source.path, 0), flags, p_abort, track.start,
                 segment_length, /*binary=*/false);
  m_decoding = true;
}

bool cue_charset_input::decode_run(audio_chunk& p_chunk, abort_callback& p_abort) {
  return m_decoder.run(p_chunk, p_abort);
}

void cue_charset_input::decode_seek(double p_seconds, abort_callback& p_abort) {
  m_decoder.seek(p_seconds, p_abort);
}

bool cue_charset_input::decode_can_seek() {
  return m_decoder.can_seek();
}

void cue_charset_input::retag_set_info(t_uint32 p_subsong, const file_info& p_info, abort_callback& p_abort) {
  (void)p_subsong;
  (void)p_info;
  (void)p_abort;
  throw exception_tagging_unsupported();
}

void cue_charset_input::retag_commit(abort_callback& p_abort) {
  (void)p_abort;
}

void cue_charset_input::remove_tags(abort_callback& p_abort) {
  (void)p_abort;
  throw exception_tagging_unsupported();
}

bool cue_charset_input::g_is_our_content_type(const char* p_content_type) {
  (void)p_content_type;
  return false;
}

bool cue_charset_input::g_is_our_path(const char* p_path, const char* p_extension) {
  (void)p_path;
  if (p_extension == nullptr) {
    return false;
  }
  const char* ext = p_extension;
  if (ext[0] == '.') {
    ++ext;
  }
  return pfc::stringEqualsI_ascii(ext, "cue");
}

GUID cue_charset_input::g_get_guid() {
  return guid_input;
}

const char* cue_charset_input::g_get_name() {
  return "CUE Charset";
}

GUID cue_charset_input::g_get_preferences_guid() {
  return preferences::guid_preferences_page;
}

// NOTE: the input_factory_t<> service registration lives in cue_input_register.cpp, which is
// compiled at the relaxed SDK warning level. Instantiating the SDK input_entry template emits
// /W4 C4100 (unreferenced parameter) warnings from the SDK headers in NDEBUG builds; keeping
// the instantiation out of this /W4 /WX translation unit avoids suppressing warnings here.

} // namespace foo_cue_charset
