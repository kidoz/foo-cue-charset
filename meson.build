project(
  'foo_cue_charset',
  'cpp',
  version: '0.2.0',
  license: 'MIT',
  default_options: [
    'cpp_std=c++latest',
    'warning_level=3',
    'b_ndebug=if-release',
  ],
)

fs = import('fs')
windows = import('windows')
cpp = meson.get_compiler('cpp')

if host_machine.system() != 'windows'
  error('foo_cue_charset targets foobar2000 on Windows/MSVC only')
endif

is_msvc = cpp.get_id() == 'msvc'
# Component-owned code: strict warnings, warnings-as-errors, conforming preprocessor.
# /external:anglebrackets + /external:W0 keep the strict /W4 /WX net on our own ("...")
# sources while silencing warnings from angle-bracket-included third-party headers (the
# foobar2000 SDK, WTL, Windows, Catch2), including those surfaced by SDK template instantiation.
component_args = is_msvc ? ['/W4', '/WX', '/permissive-', '/EHsc', '/utf-8', '/external:anglebrackets', '/external:W0'] : ['-Wall', '-Wextra', '-Wpedantic', '-Werror']
# Control Flow Guard for the distributed DLL; needs both compile and link flags.
component_hardening_args = is_msvc ? ['/guard:cf'] : []
component_hardening_link_args = is_msvc ? ['/guard:cf'] : []
# Third-party SDK sources: relaxed warnings, never -Werror.
sdk_args = is_msvc ? ['/W3', '/permissive-', '/EHsc'] : ['-Wall', '-Wextra']

project_defs = [
  '_WINDOWS',
  'UNICODE',
  '_UNICODE',
  'NOMINMAX',
  'WIN32_LEAN_AND_MEAN',
]
define_prefix = is_msvc ? '/D' : '-D'
compile_defs = []
foreach define : project_defs
  compile_defs += define_prefix + define
endforeach
compat_args = is_msvc ? ['/FI' + meson.project_source_root() / 'src/fb2k_meson_compat.hpp'] : []

version_def = define_prefix + 'FOO_CUE_CHARSET_VERSION="' + meson.project_version() + '"'

build_component = get_option('build_component')

src_inc = include_directories('src')

# --- SDK-independent support code (unit-testable as plain C++) ------------------------
# Encoding module plus the pure path helpers used by the CUE FILE resolver.
encoding_sources = files(
  'src/encoding/utf.cpp',
  'src/encoding/decoder.cpp',
  'src/path_util.cpp',
)

# Catch2 from the WrapDB wrap; include_type system keeps its headers out of /W4 /WX.
catch2_dep = dependency(
  'catch2-with-main',
  version: '>=3.0.0',
  include_type: 'system',
  default_options: ['tests=false', 'install=false'],
)

encoding_test_sources = {
  'test_utf': 'tests/test_utf.cpp',
  'test_decoder': 'tests/test_decoder.cpp',
  'test_cue_text': 'tests/test_cue_text.cpp',
  'test_path_util': 'tests/test_path_util.cpp',
}

foreach name, src : encoding_test_sources
  exe = executable(
    name,
    [files(src)] + encoding_sources,
    include_directories: src_inc,
    cpp_args: component_args + compile_defs,
    dependencies: [catch2_dep],
  )
  test(name, exe)
endforeach

# --- foobar2000 component DLL ---------------------------------------------------------
if build_component
  sdk_path = get_option('sdk_path')

  if sdk_path == 'wrap'
    fb2k_sdk_subproject = subproject('fb2k-sdk')
    fb2k_sdk_dep = fb2k_sdk_subproject.get_variable('fb2k_sdk_dep')
  else
    if not fs.exists(sdk_path / 'foobar2000/SDK/input.h')
      error('foobar2000 SDK not found at ' + sdk_path + '. Extract it there or configure with -Dsdk_path=wrap.')
    endif

    sdk_inc = include_directories(
      sdk_path,
      sdk_path / 'foobar2000',
      sdk_path / 'foobar2000/SDK',
      sdk_path / 'foobar2000/helpers',
      sdk_path / 'foobar2000/shared',
      sdk_path / 'pfc',
    )

    pfc_sources = files(
      sdk_path / 'pfc/audio_math.cpp',
      sdk_path / 'pfc/audio_sample.cpp',
      sdk_path / 'pfc/base64.cpp',
      sdk_path / 'pfc/bigmem.cpp',
      sdk_path / 'pfc/bit_array.cpp',
      sdk_path / 'pfc/bsearch.cpp',
      sdk_path / 'pfc/charDownConvert.cpp',
      sdk_path / 'pfc/cpuid.cpp',
      sdk_path / 'pfc/crashWithMessage.cpp',
      sdk_path / 'pfc/filehandle.cpp',
      sdk_path / 'pfc/filetimetools.cpp',
      sdk_path / 'pfc/guid.cpp',
      sdk_path / 'pfc/other.cpp',
      sdk_path / 'pfc/pathUtils.cpp',
      # pfc-fb2k-hooks.cpp omitted: utility.cpp already defines the same pfc hooks (avoids LNK2005).
      sdk_path / 'pfc/printf.cpp',
      sdk_path / 'pfc/SmartStrStr.cpp',
      sdk_path / 'pfc/sort.cpp',
      sdk_path / 'pfc/splitString2.cpp',
      sdk_path / 'pfc/stdafx.cpp',
      sdk_path / 'pfc/string-compare.cpp',
      sdk_path / 'pfc/string-conv-lite.cpp',
      sdk_path / 'pfc/string-lite.cpp',
      sdk_path / 'pfc/string_base.cpp',
      sdk_path / 'pfc/string_conv.cpp',
      sdk_path / 'pfc/threads.cpp',
      sdk_path / 'pfc/timers.cpp',
      sdk_path / 'pfc/unicode-normalize.cpp',
      sdk_path / 'pfc/utf8.cpp',
      sdk_path / 'pfc/wildcard.cpp',
      sdk_path / 'pfc/win-objects.cpp',
    )

    fb2k_sdk_sources = files(
      sdk_path / 'foobar2000/SDK/abort_callback.cpp',
      sdk_path / 'foobar2000/SDK/advconfig.cpp',
      sdk_path / 'foobar2000/SDK/album_art.cpp',
      sdk_path / 'foobar2000/SDK/app_close_blocker.cpp',
      sdk_path / 'foobar2000/SDK/audio_chunk.cpp',
      sdk_path / 'foobar2000/SDK/audio_chunk_channel_config.cpp',
      sdk_path / 'foobar2000/SDK/cfg_var.cpp',
      sdk_path / 'foobar2000/SDK/cfg_var_legacy.cpp',
      sdk_path / 'foobar2000/SDK/chapterizer.cpp',
      sdk_path / 'foobar2000/SDK/commandline.cpp',
      sdk_path / 'foobar2000/SDK/commonObjects.cpp',
      sdk_path / 'foobar2000/SDK/completion_notify.cpp',
      sdk_path / 'foobar2000/SDK/componentversion.cpp',
      sdk_path / 'foobar2000/SDK/configStore.cpp',
      sdk_path / 'foobar2000/SDK/config_io_callback.cpp',
      sdk_path / 'foobar2000/SDK/config_object.cpp',
      sdk_path / 'foobar2000/SDK/console.cpp',
      sdk_path / 'foobar2000/SDK/dsp.cpp',
      sdk_path / 'foobar2000/SDK/dsp_manager.cpp',
      sdk_path / 'foobar2000/SDK/file_cached_impl.cpp',
      sdk_path / 'foobar2000/SDK/file_info.cpp',
      sdk_path / 'foobar2000/SDK/file_info_const_impl.cpp',
      sdk_path / 'foobar2000/SDK/file_info_impl.cpp',
      sdk_path / 'foobar2000/SDK/file_info_merge.cpp',
      sdk_path / 'foobar2000/SDK/file_operation_callback.cpp',
      sdk_path / 'foobar2000/SDK/filesystem.cpp',
      sdk_path / 'foobar2000/SDK/filesystem_helper.cpp',
      sdk_path / 'foobar2000/SDK/foosort.cpp',
      sdk_path / 'foobar2000/SDK/fsItem.cpp',
      sdk_path / 'foobar2000/SDK/guids.cpp',
      sdk_path / 'foobar2000/SDK/hasher_md5.cpp',
      sdk_path / 'foobar2000/SDK/image.cpp',
      sdk_path / 'foobar2000/SDK/input.cpp',
      sdk_path / 'foobar2000/SDK/input_file_type.cpp',
      sdk_path / 'foobar2000/SDK/link_resolver.cpp',
      sdk_path / 'foobar2000/SDK/mainmenu.cpp',
      sdk_path / 'foobar2000/SDK/main_thread_callback.cpp',
      sdk_path / 'foobar2000/SDK/mem_block_container.cpp',
      sdk_path / 'foobar2000/SDK/menu_helpers.cpp',
      sdk_path / 'foobar2000/SDK/menu_item.cpp',
      sdk_path / 'foobar2000/SDK/menu_manager.cpp',
      sdk_path / 'foobar2000/SDK/metadb.cpp',
      sdk_path / 'foobar2000/SDK/metadb_handle.cpp',
      sdk_path / 'foobar2000/SDK/metadb_handle_list.cpp',
      sdk_path / 'foobar2000/SDK/output.cpp',
      sdk_path / 'foobar2000/SDK/packet_decoder.cpp',
      sdk_path / 'foobar2000/SDK/playable_location.cpp',
      sdk_path / 'foobar2000/SDK/playback_control.cpp',
      sdk_path / 'foobar2000/SDK/playlist.cpp',
      sdk_path / 'foobar2000/SDK/playlist_loader.cpp',
      sdk_path / 'foobar2000/SDK/popup_message.cpp',
      sdk_path / 'foobar2000/SDK/preferences_page.cpp',
      sdk_path / 'foobar2000/SDK/replaygain.cpp',
      sdk_path / 'foobar2000/SDK/replaygain_info.cpp',
      sdk_path / 'foobar2000/SDK/service.cpp',
      sdk_path / 'foobar2000/SDK/stdafx.cpp',
      sdk_path / 'foobar2000/SDK/tag_processor.cpp',
      sdk_path / 'foobar2000/SDK/tag_processor_id3v2.cpp',
      sdk_path / 'foobar2000/SDK/threaded_process.cpp',
      sdk_path / 'foobar2000/SDK/titleformat.cpp',
      sdk_path / 'foobar2000/SDK/track_property.cpp',
      sdk_path / 'foobar2000/SDK/ui.cpp',
      sdk_path / 'foobar2000/SDK/ui_element.cpp',
      sdk_path / 'foobar2000/SDK/utility.cpp',
    )

    fb2k_helper_sources = files(
      sdk_path / 'foobar2000/helpers/cue_parser.cpp',
      sdk_path / 'foobar2000/helpers/cue_creator.cpp',
      sdk_path / 'foobar2000/helpers/cue_parser_embedding.cpp',
      sdk_path / 'foobar2000/helpers/cuesheet_index_list.cpp',
      sdk_path / 'foobar2000/helpers/input_helpers.cpp',
      sdk_path / 'foobar2000/helpers/input_helper_cue.cpp',
      sdk_path / 'foobar2000/helpers/file_list_helper.cpp',
      sdk_path / 'foobar2000/helpers/readers.cpp',
    )

    pfc_lib = static_library(
      'pfc',
      pfc_sources,
      include_directories: sdk_inc,
      cpp_args: sdk_args + compile_defs + compat_args,
      cpp_pch: sdk_path / 'pfc/pfc-lite.h',
      override_options: ['cpp_std=c++20'],
    )

    fb2k_sdk_lib = static_library(
      'foobar2000_SDK',
      fb2k_sdk_sources,
      include_directories: sdk_inc,
      dependencies: declare_dependency(link_with: pfc_lib),
      cpp_args: sdk_args + compile_defs + compat_args,
      cpp_pch: sdk_path / 'foobar2000/SDK/foobar2000-sdk-pch.h',
      override_options: ['cpp_std=c++20'],
    )

    fb2k_helpers_lib = static_library(
      'foobar2000_helpers',
      fb2k_helper_sources,
      include_directories: sdk_inc,
      dependencies: declare_dependency(link_with: [pfc_lib, fb2k_sdk_lib]),
      cpp_args: sdk_args + compile_defs + compat_args,
      override_options: ['cpp_std=c++20'],
    )

    component_client_lib = static_library(
      'foobar2000_component_client',
      files(sdk_path / 'foobar2000/foobar2000_component_client/component_client.cpp'),
      include_directories: sdk_inc,
      dependencies: declare_dependency(link_with: [pfc_lib, fb2k_sdk_lib]),
      cpp_args: sdk_args + compile_defs + compat_args,
      override_options: ['cpp_std=c++20'],
    )

    shared_name = host_machine.cpu_family() == 'x86' ? 'shared-Win32' : 'shared-x64'
    shared_dep = cpp.find_library(
      shared_name,
      dirs: meson.project_source_root() / sdk_path / 'foobar2000/shared',
      required: true,
    )

    fb2k_sdk_dep = declare_dependency(
      include_directories: sdk_inc,
      link_with: [component_client_lib, fb2k_helpers_lib, fb2k_sdk_lib, pfc_lib],
      dependencies: [shared_dep],
    )
  endif

  system_libs = [
    cpp.find_library('user32'),
    cpp.find_library('comctl32'),
    cpp.find_library('gdi32'),
    cpp.find_library('shlwapi'),
    cpp.find_library('ole32'),
    cpp.find_library('uuid'),
    cpp.find_library('winmm'),
  ]

  component_sources = files(
    'src/component.cpp',
    'src/cue_input.cpp',
    'src/cue_sheet.cpp',
    'src/config.cpp',
    'src/preferences.cpp',
  )

  # The SDK input service registration instantiates SDK templates that emit /W4 C4100 warnings
  # from the SDK headers in NDEBUG builds. Compile that one TU at the relaxed SDK warning level
  # and pull it in with link_whole so the static service-factory initializer is not stripped.
  component_register_lib = static_library(
    'foo_cue_charset_register',
    files('src/cue_input_register.cpp'),
    include_directories: src_inc,
    dependencies: [fb2k_sdk_dep],
    cpp_args: sdk_args + compile_defs + compat_args,
    override_options: ['cpp_std=c++20'],
  )

  component_resources = windows.compile_resources('res/preferences.rc')

  component = shared_library(
    'foo_cue_charset',
    component_sources,
    encoding_sources,
    component_resources,
    include_directories: src_inc,
    dependencies: [fb2k_sdk_dep, system_libs],
    link_whole: [component_register_lib],
    cpp_args: component_args + component_hardening_args + compile_defs + compat_args + [version_def],
    link_args: component_hardening_link_args,
    name_prefix: '',
    name_suffix: 'dll',
    install: false,
  )

  # The SDK parser uses shared.dll from the player. Keep encoding tests independent
  # and enable this additional suite only when an explicit runtime path is supplied.
  foobar2000_path = get_option('foobar2000_path')
  if foobar2000_path != ''
    if not fs.exists(foobar2000_path / 'shared.dll')
      error('foobar2000_path must contain shared.dll from a matching-architecture player installation')
    endif
    sdk_test = executable(
      'test_cue_sheet',
      files('tests/test_cue_sheet.cpp', 'src/cue_sheet.cpp'),
      encoding_sources,
      include_directories: src_inc,
      dependencies: [catch2_dep, fb2k_sdk_dep, system_libs],
      cpp_args: component_args + compile_defs + compat_args,
    )
    sdk_test_env = environment()
    sdk_test_env.prepend('PATH', foobar2000_path)
    test('test_cue_sheet', sdk_test, env: sdk_test_env)
  else
    message('SDK parser tests disabled; set -Dfoobar2000_path to the player directory to enable them')
  endif
endif

# --- Developer tooling: format / tidy / package ---------------------------------------
format_files = [
  'src/component.cpp',
  'src/cue_input.cpp',
  'src/cue_input.hpp',
  'src/cue_sheet.cpp',
  'src/cue_sheet.hpp',
  'src/cue_input_register.cpp',
  'src/config.cpp',
  'src/config.hpp',
  'src/preferences.cpp',
  'src/preferences.hpp',
  'src/fb2k_meson_compat.hpp',
  'src/resource.h',
  'src/path_util.cpp',
  'src/path_util.hpp',
  'src/encoding/utf.cpp',
  'src/encoding/abort.hpp',
  'src/encoding/utf.hpp',
  'src/encoding/decoder.cpp',
  'src/encoding/decoder.hpp',
  'tests/test_utf.cpp',
  'tests/test_decoder.cpp',
  'tests/test_cue_text.cpp',
  'tests/test_path_util.cpp',
  'tests/test_cue_sheet.cpp',
  'tests/fixtures_cyrillic.hpp',
]

format_list = '@("' + '", "'.join(format_files) + '")'

clang_format = find_program('clang-format', required: false)
if clang_format.found()
  format_script = 'Set-Location $env:MESON_SOURCE_ROOT; $files = ' + format_list + '; & clang-format -i $files; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }'
  format_check_script = 'Set-Location $env:MESON_SOURCE_ROOT; $files = ' + format_list + '; & clang-format --dry-run --Werror $files; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }'
  run_target(
    'format',
    command: ['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command', format_script],
  )
  run_target(
    'format-check',
    command: ['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command', format_check_script],
  )
endif

# clang-tidy runs against component-owned sources only (the SDK is third-party).
tidy_files = [
  'src/component.cpp',
  'src/cue_input.cpp',
  'src/cue_sheet.cpp',
  'src/config.cpp',
  'src/preferences.cpp',
  'src/path_util.cpp',
  'src/encoding/utf.cpp',
  'src/encoding/decoder.cpp',
  'tests/test_utf.cpp',
  'tests/test_decoder.cpp',
  'tests/test_cue_text.cpp',
  'tests/test_path_util.cpp',
]
if build_component and get_option('foobar2000_path') != ''
  tidy_files += 'tests/test_cue_sheet.cpp'
endif
tidy_list = '@("' + '", "'.join(tidy_files) + '")'

clang_tidy = find_program('clang-tidy', required: false)
if clang_tidy.found()
  tidy_script = 'Set-Location $env:MESON_SOURCE_ROOT; $files = ' + tidy_list + '; foreach ($file in $files) { & clang-tidy -p $env:MESON_BUILD_ROOT --extra-arg=-Wno-unused-command-line-argument $file; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }'
  run_target(
    'clang-tidy',
    command: ['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command', tidy_script],
  )
endif

run_target(
  'package',
  command: [
    'powershell',
    '-NoProfile',
    '-ExecutionPolicy',
    'Bypass',
    '-Command',
    '$zip = Join-Path $env:MESON_SOURCE_ROOT "foo_cue_charset.zip"; $component = Join-Path $env:MESON_SOURCE_ROOT "foo_cue_charset.fb2k-component"; Compress-Archive -Path (Join-Path $env:MESON_BUILD_ROOT "foo_cue_charset.dll") -DestinationPath $zip -Force; Move-Item -LiteralPath $zip -Destination $component -Force',
  ],
)
