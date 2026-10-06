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
    // Validate against the most recent previous track referencing the same file, wherever it
    // appears in the sheet — not only the adjacent one — so interleaved FILE blocks cannot
    // smuggle in non-increasing indexes or a binary/decoded mode switch. Transitivity of the
    // strict ordering makes the most recent occurrence the only one that needs checking.
    for (auto it = tracks.rbegin(); it != tracks.rend(); ++it) {
      if (pfc::stringEqualsI_utf8(it->file, track.file)) {
        if (it->binary != track.binary) {
          throw exception_io_data("CUE Charset: conflicting FILE types for the same audio file");
        }
        checked_track_length(it->start, track.start);
        break;
      }
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
