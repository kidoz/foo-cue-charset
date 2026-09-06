# Run the manual acceptance checks

Use this procedure to assess a candidate package in a running foobar2000 2.25+ x64 instance.
It complements the automated suites; the current [matrix](../reference/player-test-matrix.md)
records expected results, with execution still pending.

## Prepare the run

1. [Build the release package](build-and-test.md#produce-a-release-package) and inspect its contents.
2. Record the commit or source snapshot, package SHA-256, Windows version, player version,
   architecture, installed audio decoders, and date in a run record.
3. Prepare both a clean player profile and an existing profile. Record which profile each result
   uses, including the decoder-priority order.
4. Keep test CUEs and audio in a dedicated test directory. Use copies for malformed-input cases.

To record the package hash:

```powershell
Get-FileHash -LiteralPath './foo_cue_charset.fb2k-component' -Algorithm SHA256
```

## Prepare the fixtures

Create a valid audio image longer than 4 minutes 12 seconds and use its exact Cyrillic filename
in a two-track CUE. This is an illustrative UTF-8 rendering of the test text:

```cue
PERFORMER "Август"
TITLE "Демоны любви"
FILE "Август - Демоны любви.flac" WAVE
  TRACK 01 AUDIO
    TITLE "Демоны любви"
    PERFORMER "Август"
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    TITLE "Ночь"
    PERFORMER "Август"
    INDEX 01 04:12:00
```

Save separate copies in Windows-1251, KOI8-R, CP866, ISO-8859-5, UTF-8 without BOM, and UTF-8
with BOM. Preserve the same text and line endings; the encoded bytes will differ. Also prepare
UTF-16 LE/BE BOM copies, a multi-FILE sheet, and FLAC, APE, WAV, and WavPack sources where
matching decoders are available.

For the BINARY cases, use a raw stereo 16-bit, 44.1 kHz PCM image with known duration. Prepare
separate missing-file, ambiguous-extension, malformed-timestamp, NUL, and undefined-byte
fixtures. Do not edit the valid fixtures to create these variants in place.

## Exercise the matrix

1. [Install and configure](install-and-configure.md) the package in the clean profile.
2. Run each [acceptance case](../reference/player-test-matrix.md), checking metadata, durations,
   playback, seeking, and the Console as applicable.
3. Repeat profile-sensitive cases in the existing profile. Change the fallback for each legacy
   encoding and record it with the result.
4. Restart the player for persistence cases; perform actual ReplayGain and Converter operations
   for their rows. Test install/uninstall and decoder priority explicitly.
5. Mark unavailable decoder/network scenarios NOT RUN with a reason. Leave no blank status that
   could be mistaken for a pass.

## Verify the read-only contract

Before adding the valid test CUEs, save a hash manifest. In this example, `cue-test-data` is the
prepared test directory under the repository root:

```powershell
Get-ChildItem -LiteralPath './cue-test-data' -Filter '*.cue' -File |
    Get-FileHash -Algorithm SHA256 |
    Select-Object Path, Hash |
    Export-Csv -LiteralPath './cue-hashes-before.csv' -NoTypeInformation
```

After open, scan, playback, and rejected tag-write attempts, compare the same files:

```powershell
Import-Csv -LiteralPath './cue-hashes-before.csv' | ForEach-Object {
    $currentHash = (Get-FileHash -LiteralPath $_.Path -Algorithm SHA256).Hash
    [pscustomobject]@{ Path = $_.Path; Unchanged = ($currentHash -eq $_.Hash) }
}
```

Every valid fixture should report `Unchanged = True`. Record errors or missing files as failures
rather than treating a missing comparison as a pass.

## Record the result

Use one row per matrix case and profile:

| Case | Profile | Encoding / decoder | Result | Evidence or reason not run |
| --- | --- | --- | --- | --- |
| Case ID | Clean or existing | Actual settings | PASS / FAIL / NOT RUN | Observations |

A passing parser suite or a successful DLL build does not replace these results. State remaining
unverified cases with any release assessment.

[Documentation home](../README.md)
