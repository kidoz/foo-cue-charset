# Build and explore the encoding tests

In this tutorial, we will build the project's encoding tests, run a Windows-1251 regression
case, and find the code that it exercises. The result is a working local test environment.

## Before you begin

Use Windows x64 with a checkout of this repository, Visual Studio 2022 Build Tools with the
MSVC v143 C++ toolchain or newer, Meson, and Ninja. The first configuration needs network access
to obtain Catch2. Open PowerShell in the repository root. Use a new `build-tutorial` directory.

This exercise uses the SDK-independent targets. A foobar2000 installation and the SDK are not
needed for these targets.

## 1. Configure the exercise

Run:

```powershell
meson setup build-tutorial --vsenv -Dbuild_component=false
```

Look for an x64 MSVC compiler and a successful Catch2 dependency check in the output.
Meson creates `build-tutorial/build.ninja`.

## 2. Build and run the tests

```powershell
meson compile -C build-tutorial
meson test -C build-tutorial --print-errorlogs
```

The summary should show four passing suites: `test_utf`, `test_decoder`, `test_cue_text`, and
`test_path_util`. There is no component DLL in this build because we disabled that target.

## 3. Inspect a legacy-encoding regression

Run just the decoder regression cases:

```powershell
.\build-tutorial\test_decoder.exe "[decoder][regression]"
```

The selected cases should pass. Open [test_decoder.cpp](../../tests/test_decoder.cpp) and find
`Undefined Windows-1251 bytes fail in automatic and forced modes`.

Notice the explicit byte `0x98` in the test input and the expected `conversion_failed` result.
Now open [decoder.cpp](../../src/encoding/decoder.cpp) and find the explicit Windows-1251 check
for that byte. The test connects the input byte to a typed conversion error.

## 4. Follow the complete document test

```powershell
.\build-tutorial\test_cue_text.exe
```

The CUE-text cases should pass. In [test_cue_text.cpp](../../tests/test_cue_text.cpp), find
`build_cue` and the CRLF test. The comparison covers the whole document, including its Cyrillic
`FILE` name and line endings.

You now have a local build that can check changes to the conversion layer. To build the DLL,
continue with [Build, test, and package](../how-to/build-and-test.md). For the reasons behind
the conversion policy, read [Why detection uses a configured fallback](../explanation/encoding-policy.md).

[Documentation home](../README.md)
