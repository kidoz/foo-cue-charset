# Troubleshoot a CUE sheet

Use these procedures after installing CUE Charset. Keep a copy of a CUE before manually
editing its text or filenames.

## Cyrillic metadata is unreadable

1. Open **Preferences → Tools → CUE Charset** and enable encoding-decision logging.
2. Add the CUE again. Check the foobar2000 Console for a `foo_cue_charset:` encoding message.
3. If the reported legacy encoding is wrong, select the encoding used by the file's author.
   Automatic mode does not choose between legacy encodings.
4. If a known legacy file is reported as UTF-8, use **Force selected encoding** with that legacy
   encoding. Apply and add the file again.
5. If there is no decision message, check [handler priority](install-and-configure.md#set-handler-priority).

Restore Automatic when Force is no longer needed, and disable logging when finished.
If the source encoding is unknown, obtain that information from the source of the CUE; a
successful decode alone does not prove that the text uses the correct alphabet.

## The referenced audio file is missing

1. Compare the CUE's quoted `FILE` name with the actual filename, including Cyrillic characters
   and relative subdirectories.
2. Confirm that the audio file is available at that location and that its decoder is installed.
3. If the basename differs, correct the CUE in an editor that preserves its encoding, or restore
   the expected audio filename.
4. If only the extension differs, read the [reference-resolution rules](../reference/behavior.md#referenced-audio).
   An ambiguity error requires an explicit filename; remove the ambiguity in the `FILE` line.

For `BINARY` references, restore the exact raw PCM image. A same-basename compressed file is
not a substitute and is not selected automatically.

## The component reports invalid data

- For an undefined Windows-1251 byte, correct the source data or select the actual source
  encoding. Byte `0x98` is rejected when interpreted as Windows-1251.
- For malformed BOM-tagged Unicode, repair or obtain a valid copy of the source file. Automatic
  mode does not fall back to legacy decoding after recognizing a Unicode BOM.
- For an embedded NUL or an input over 16 MiB, obtain a valid text CUE within the limit.
- For a timestamp error, ensure consecutive tracks in one source have strictly increasing
  `INDEX 01` times. Different source files can each start at zero.
- For a syntax error, check the `FILE`, `TRACK`, and `INDEX` lines. The component uses the SDK's
  CUE grammar; diagnostic output deliberately does not echo arbitrary CUE contents.

## Cancellation is delayed

Conversion polls cancellation during processing. A running SDK parser call must return before
the next cancellation check. See the [cancellation design](../explanation/architecture.md#cancellation)
for this boundary. When reporting a delay, record the file size and operation without posting
private CUE contents or paths unnecessarily.

[Documentation home](../README.md)
