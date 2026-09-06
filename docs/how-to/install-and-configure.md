# Install and configure CUE Charset

Use this guide when you have `foo_cue_charset.fb2k-component` and want to open a legacy-encoded
CUE in foobar2000 2.25+ x64. To create the package, see [Build, test, and package](build-and-test.md).

## Install the package

1. Open foobar2000's **Preferences → Components → Install...**.
2. Select `foo_cue_charset.fb2k-component`, apply the change, and restart when prompted.
3. Check that **CUE Charset** appears in Components and that its page appears under
   **Preferences → Tools → CUE Charset**.

## Select the source encoding

1. Open **Preferences → Tools → CUE Charset**.
2. Leave the detection mode on **Automatic** for Unicode CUEs and legacy CUEs whose encoding
   matches the configured fallback.
3. Set **Legacy fallback encoding** to the encoding used to save your CUE. The default is
   **Windows-1251**; the other choices are **KOI8-R**, **CP866**, and **ISO-8859-5**.
4. Apply the settings, then add the CUE again to check its metadata and referenced filename.

If you know that a legacy file is being interpreted as UTF-8, select **Force selected encoding**
and the known source encoding. Force applies to every CUE handled by the component, including
Unicode files; restore Automatic after using it if your collection contains mixed encodings.

## Set handler priority

If the built-in reader handles the file first, open **Preferences → Playback → Decoding** and
place **CUE Charset** above the built-in CUE reader. Add the CUE again and check the result.

For unreadable text or an unresolved audio file, use [Troubleshoot a CUE sheet](troubleshoot.md).
See [Behavior, preferences, and limits](../reference/behavior.md) for exact defaults and rules.

[Documentation home](../README.md)
