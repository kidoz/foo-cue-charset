#include "cue_sheet.hpp"

#include <helpers/cue_parser.h>

#include <cmath>

namespace foo_cue_charset {

double checked_track_length(double start, double next_start) {
  if (!std::isfinite(start) || !std::isfinite(next_start) || start < 0.0 || next_start <= start) {
    throw exception_io_data("CUE Charset: track indexes in the same audio file must increase");
  }
  return next_start - start;
}

std::vector<cue_track> parse_cue_sheet(const char* text, abort_callback& abort) {
  abort.check();
  cue_parser::t_cue_entry_list entries;
  cue_parser::parse(text, entries);
  abort.check();

  std::vector<cue_track> tracks;
  for (const auto& entry : entries) {
    abort.check();
    cue_track track;
    track.number = entry.m_track_number;
    track.file = entry.m_file;
    track.binary = pfc::stringEqualsI_ascii(entry.m_fileType, "BINARY");
    track.start = entry.m_indexes.start();
    if (!tracks.empty() && pfc::stringEqualsI_utf8(tracks.back().file, track.file)) {
      if (tracks.back().binary != track.binary) {
        throw exception_io_data("CUE Charset: conflicting FILE types for the same audio file");
      }
      checked_track_length(tracks.back().start, track.start);
    }
    tracks.push_back(std::move(track));
  }
  abort.check();
  return tracks;
}

void read_cue_metadata(const char* text, unsigned number, file_info& info, abort_callback& abort) {
  abort.check();
  cue_parser::parse_info(text, info, number);
  abort.check();
}

} // namespace foo_cue_charset
