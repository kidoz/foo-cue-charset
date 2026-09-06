# Why detection uses a configured fallback

A byte stream does not reliably identify its own legacy encoding. Several single-byte pages
can decode the same bytes into different characters without reporting an error. A successful
conversion alone cannot tell whether a title or filename is correct.

CUE Charset therefore uses explicit Unicode evidence first: a supported BOM, then strict
UTF-8 validation. With neither, it applies one configured legacy fallback. This makes the same
input and settings produce the same result on every supported Windows installation, independent
of the process locale or system ANSI code page.

## Automatic and Force serve different situations

Automatic preserves Unicode documents while making a collection with a known legacy encoding
convenient to use. It does not statistically distinguish Windows-1251, KOI8-R, CP866, and
ISO-8859-5. Windows-1251 is the initial fallback, not a claim about an individual file.

A legacy byte sequence can accidentally be valid UTF-8. Force exists for that case: a user
who knows the source encoding can bypass detection. The UI offers legacy encodings, so Force
also interprets any Unicode BOM bytes through that legacy page. This is why it should be used
deliberately when a collection contains mixed encodings.

## Whole-document conversion matters

Fixing only displayed metadata would leave the `FILE` directive in the wrong encoding. The
player could then display a correct album title but fail to locate its audio. Converting the
whole CUE before parsing treats titles, comments, and filesystem references consistently while
preserving line structure.

## Errors should preserve the problem

Replacing an invalid byte with `?` or U+FFFD would hide damaged text and could change a filename.
The converter validates Unicode scalars and rejects embedded NUL so the SDK cannot silently
see a truncated document.

Win32 strict-conversion flags do not cover every legacy mapping rule: Windows accepts CP1251
byte `0x98` as U+0098 even though [Microsoft's published mapping](https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1251.TXT)
leaves it undefined. The converter
checks that byte explicitly for Windows-1251. The test suite also verifies that this rule does
not reject the same byte in other supported pages or within a valid UTF-8 sequence.

See [Behavior](../reference/behavior.md#encoding-selection) for the selection table and
[Troubleshoot a CUE sheet](../how-to/troubleshoot.md) for changing an incorrect fallback.

[Documentation home](../README.md)
