# Build, test, and package

Run these commands from the repository root in PowerShell on Windows x64. Install the
[build prerequisites](../reference/build.md#prerequisites) first.

## Configure a component build

```powershell
just setup
```

This prepares the pinned SDK and WTL downloads, resolves Catch2, and configures `build/`.
For an existing build that needs regeneration, use:

```powershell
just reconfigure
```

## Run the quality checks

```powershell
just check
```

This builds the component and tests, checks formatting, runs clang-tidy with warnings-as-errors,
and runs the tests. The default configuration has four SDK-independent suites.

To apply the configured formatting before checking again:

```powershell
just format
just check
```

## Include SDK parser tests

With a matching x64 foobar2000 installation, configure the path containing its `shared.dll`:

```powershell
meson configure build '-Dfoobar2000_path=C:/Program Files/foobar2000'
just check
```

Replace the example path if the player is installed elsewhere. The additional `test_cue_sheet`
suite calls the SDK parser without launching the player. It does not exercise playback or core
service registration. For those checks, use [Run the manual acceptance checks](verify-in-player.md).

## Produce a release package

```powershell
just release
```

The recipe builds Release, runs its tests, and writes `foo_cue_charset.fb2k-component` in the
repository root. Debug and Release options are independent. To include the SDK suite in Release:

```powershell
just build-release
meson configure build-release '-Dfoobar2000_path=C:/Program Files/foobar2000'
just release
```

Inspect the package after the build:

```powershell
Add-Type -AssemblyName System.IO.Compression.FileSystem
$packagePath = (Resolve-Path -LiteralPath './foo_cue_charset.fb2k-component').Path
$archive = [System.IO.Compression.ZipFile]::OpenRead($packagePath)
try {
    $archive.Entries | Select-Object FullName, Length
} finally {
    $archive.Dispose()
}
```

The only entry must be `foo_cue_charset.dll`. Package creation does not establish player acceptance.

## Build the SDK-independent tests only

```powershell
meson setup build-tests-only --vsenv -Dbuild_component=false
meson compile -C build-tests-only
meson test -C build-tests-only --print-errorlogs
```

This configuration still requires Windows and its Unicode APIs. Use the direct Meson test
commands for this configuration; the full `just check` static-analysis list includes component sources.

## Use separate build directories

If an existing build uses a different Meson version, first try `just reconfigure`. To keep it
while creating a new build:

```powershell
just build_dir=build-local setup
just build_dir=build-local check
just release_dir=build-local-release release
```

All package recipes write the same package filename at the repository root. See the
[command reference](../reference/build.md) for recipe dependencies and option defaults.

[Documentation home](../README.md)
