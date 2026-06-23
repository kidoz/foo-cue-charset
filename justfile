set shell := ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command"]

build_dir := "build"
release_dir := "build-release"
sdk_path := "wrap"
sdk_archive := "subprojects/packagecache/SDK-2025-03-07.7z"
sdk_url := "https://www.foobar2000.org/downloads/SDK-2025-03-07.7z"
sdk_sha256 := "ccda3c5840e66e0e28a7e4fe36407c4e78581aa30c40c362a188fcbaae799a3e"
# WTL is required by the official SDK helper sources (cue_parser, input_helpers, ...) via their
# StdAfx/foobar2000+atl.h. It is a header-only, freely redistributable (MS-PL) library.
wtl_archive := "subprojects/packagecache/wtl-10.0.10320.nupkg"
wtl_url := "https://www.nuget.org/api/v2/package/wtl/10.0.10320"
wtl_sha256 := "28670adb25c05772a4b5d7c598c98b1567587b377d689a792845b026768f4d3a"

default:
    just --list

# Resolve the SDK + Catch2 wraps, then configure the debug build directory.
setup: sdk-wrap
    meson setup {{build_dir}} --vsenv -Dsdk_path={{sdk_path}}

reconfigure: sdk-wrap
    meson setup {{build_dir}} --reconfigure --vsenv -Dsdk_path={{sdk_path}}

# Build the component + tests in the default (debugoptimized) configuration.
build:
    if (!(Test-Path "{{build_dir}}/build.ninja")) { meson setup {{build_dir}} --vsenv -Dsdk_path={{sdk_path}} } else { meson configure {{build_dir}} -Dsdk_path={{sdk_path}} }
    meson compile -C {{build_dir}}

build-debug: sdk-wrap
    if (!(Test-Path "{{build_dir}}/build.ninja")) { meson setup {{build_dir}} --vsenv -Dsdk_path={{sdk_path}} } else { meson configure {{build_dir}} -Dsdk_path={{sdk_path}} }
    meson compile -C {{build_dir}}

build-release: sdk-wrap
    if (!(Test-Path "{{release_dir}}/build.ninja")) { meson setup {{release_dir}} --vsenv -Dsdk_path={{sdk_path}} --buildtype=release } else { meson configure {{release_dir}} -Dsdk_path={{sdk_path}} }
    meson compile -C {{release_dir}}

test: build
    meson test -C {{build_dir}}

test-release: build-release
    meson test -C {{release_dir}}

# Build (warnings-as-errors), verify formatting, run static analysis, and run the unit tests.
check: build format-check tidy test

format: build
    meson compile -C {{build_dir}} format

format-check: build
    meson compile -C {{build_dir}} format-check

tidy: build
    meson compile -C {{build_dir}} clang-tidy

package: build
    meson compile -C {{build_dir}} package

package-release: build-release
    meson compile -C {{release_dir}} package

# Full release pipeline: build Release, run tests, and produce foo_cue_charset.fb2k-component.
release: build-release test-release package-release

clean:
    if (Test-Path "{{build_dir}}") { Remove-Item -LiteralPath "{{build_dir}}" -Recurse -Force }
    if (Test-Path "{{release_dir}}") { Remove-Item -LiteralPath "{{release_dir}}" -Recurse -Force }

# Download + extract the SDK into ./third_party (standalone, not used by the build).
sdk:
    New-Item -ItemType Directory -Force -Path "third_party" | Out-Null
    if (!(Test-Path "third_party/SDK-2025-03-07.7z")) { Invoke-WebRequest -Uri "{{sdk_url}}" -OutFile "third_party/SDK-2025-03-07.7z" }
    if (!(Test-Path "third_party/fb2k-sdk/foobar2000/SDK/input.h")) { 7z x "third_party/SDK-2025-03-07.7z" "-othird_party/fb2k-sdk" -y }

# Download + extract the SDK and WTL into subprojects/fb2k-sdk and copy the project Meson packagefiles in.
sdk-wrap:
    New-Item -ItemType Directory -Force -Path "subprojects/packagecache" | Out-Null
    if (!(Test-Path "{{sdk_archive}}")) { Invoke-WebRequest -Uri "{{sdk_url}}" -OutFile "{{sdk_archive}}" }
    $h = (Get-FileHash "{{sdk_archive}}" -Algorithm SHA256).Hash.ToLower(); if ($h -ne "{{sdk_sha256}}") { Remove-Item -LiteralPath "{{sdk_archive}}" -Force; throw "SDK archive SHA-256 mismatch: got $h, expected {{sdk_sha256}}" }
    if (!(Test-Path "subprojects/fb2k-sdk/foobar2000/SDK/input.h")) { New-Item -ItemType Directory -Force -Path "subprojects/fb2k-sdk" | Out-Null; 7z x "{{sdk_archive}}" "-osubprojects/fb2k-sdk" -y }
    if (!(Test-Path "{{wtl_archive}}")) { Invoke-WebRequest -Uri "{{wtl_url}}" -OutFile "{{wtl_archive}}" }
    $hw = (Get-FileHash "{{wtl_archive}}" -Algorithm SHA256).Hash.ToLower(); if ($hw -ne "{{wtl_sha256}}") { Remove-Item -LiteralPath "{{wtl_archive}}" -Force; throw "WTL archive SHA-256 mismatch: got $hw, expected {{wtl_sha256}}" }
    if (!(Test-Path "subprojects/fb2k-sdk/wtl/atlapp.h")) { New-Item -ItemType Directory -Force -Path "subprojects/fb2k-sdk/wtl" | Out-Null; 7z e "{{wtl_archive}}" "lib/native/include/*.h" "-osubprojects/fb2k-sdk/wtl" -y }
    Copy-Item -Path "subprojects/packagefiles/fb2k-sdk/*" -Destination "subprojects/fb2k-sdk" -Recurse -Force
