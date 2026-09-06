#pragma once

#include <SDK/foobar2000-lite.h>
#include <SDK/file_info.h>

#include <vector>

namespace foo_cue_charset {

struct cue_track {
  unsigned number = 0;
  pfc::string8 file;
  bool binary = false;
  double start = 0.0;
};

//! SDK parsing without filesystem access. The SDK parser has no cancellation hook;
//! check before and after each parse and while validating its result.
std::vector<cue_track> parse_cue_sheet(const char* text, abort_callback& abort);
void read_cue_metadata(const char* text, unsigned number, file_info& info, abort_callback& abort);

//! Only terminal tracks may use the "to EOF" sentinel. Adjacent tracks in the same
//! source must have strictly increasing starts, including after path resolution.
double checked_track_length(double start, double next_start);

} // namespace foo_cue_charset
