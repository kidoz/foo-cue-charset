project(
  'fb2k-sdk',
  'cpp',
  default_options: [
    'cpp_std=c++20',
    'warning_level=2',
  ],
)

cpp = meson.get_compiler('cpp')
is_msvc = cpp.get_id() == 'msvc'
# The vendored SDK / pfc / helpers are third-party code: compile them at a lower warning
# level than the component (warning_level=2 -> /W3), and never treat their warnings as errors.
sdk_args = is_msvc ? ['/permissive-', '/EHsc'] : ['-Wall', '-Wextra']

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
compat_args = is_msvc ? ['/FI' + meson.current_source_dir() / 'fb2k_meson_compat.hpp'] : []

sdk_inc = include_directories(
  '.',
  'foobar2000',
  'foobar2000/SDK',
  'foobar2000/helpers',
  'foobar2000/shared',
  'pfc',
  # WTL headers, extracted next to the SDK by `just sdk-wrap`. The official SDK helpers
  # (cue_parser, input_helpers, ...) include them through their StdAfx/foobar2000+atl.h.
  'wtl',
)

pfc_sources = files(
  'pfc/audio_math.cpp',
  'pfc/audio_sample.cpp',
  'pfc/base64.cpp',
  'pfc/bigmem.cpp',
  'pfc/bit_array.cpp',
  'pfc/bsearch.cpp',
  'pfc/charDownConvert.cpp',
  'pfc/cpuid.cpp',
  'pfc/crashWithMessage.cpp',
  'pfc/filehandle.cpp',
  'pfc/filetimetools.cpp',
  'pfc/guid.cpp',
  'pfc/other.cpp',
  'pfc/pathUtils.cpp',
  # pfc-fb2k-hooks.cpp is intentionally omitted: it only re-exports pfc::crashHook and
  # pfc::winFormatSystemErrorMessageHook, which foobar2000/SDK/utility.cpp also defines.
  # Compiling both produces LNK2005 once the SDK helpers pull in utility.cpp.
  'pfc/printf.cpp',
  'pfc/SmartStrStr.cpp',
  'pfc/sort.cpp',
  'pfc/splitString2.cpp',
  'pfc/stdafx.cpp',
  'pfc/string-compare.cpp',
  'pfc/string-conv-lite.cpp',
  'pfc/string-lite.cpp',
  'pfc/string_base.cpp',
  'pfc/string_conv.cpp',
  'pfc/threads.cpp',
  'pfc/timers.cpp',
  'pfc/unicode-normalize.cpp',
  'pfc/utf8.cpp',
  'pfc/wildcard.cpp',
  'pfc/win-objects.cpp',
)

fb2k_sdk_sources = files(
  'foobar2000/SDK/abort_callback.cpp',
  'foobar2000/SDK/advconfig.cpp',
  'foobar2000/SDK/album_art.cpp',
  'foobar2000/SDK/app_close_blocker.cpp',
  'foobar2000/SDK/audio_chunk.cpp',
  'foobar2000/SDK/audio_chunk_channel_config.cpp',
  'foobar2000/SDK/cfg_var.cpp',
  'foobar2000/SDK/cfg_var_legacy.cpp',
  'foobar2000/SDK/chapterizer.cpp',
  'foobar2000/SDK/commandline.cpp',
  'foobar2000/SDK/commonObjects.cpp',
  'foobar2000/SDK/completion_notify.cpp',
  'foobar2000/SDK/componentversion.cpp',
  'foobar2000/SDK/configStore.cpp',
  'foobar2000/SDK/config_io_callback.cpp',
  'foobar2000/SDK/config_object.cpp',
  'foobar2000/SDK/console.cpp',
  'foobar2000/SDK/dsp.cpp',
  'foobar2000/SDK/dsp_manager.cpp',
  'foobar2000/SDK/file_cached_impl.cpp',
  'foobar2000/SDK/file_info.cpp',
  'foobar2000/SDK/file_info_const_impl.cpp',
  'foobar2000/SDK/file_info_impl.cpp',
  'foobar2000/SDK/file_info_merge.cpp',
  'foobar2000/SDK/file_operation_callback.cpp',
  'foobar2000/SDK/filesystem.cpp',
  'foobar2000/SDK/filesystem_helper.cpp',
  'foobar2000/SDK/foosort.cpp',
  'foobar2000/SDK/fsItem.cpp',
  'foobar2000/SDK/guids.cpp',
  'foobar2000/SDK/hasher_md5.cpp',
  'foobar2000/SDK/image.cpp',
  'foobar2000/SDK/input.cpp',
  'foobar2000/SDK/input_file_type.cpp',
  'foobar2000/SDK/link_resolver.cpp',
  'foobar2000/SDK/mainmenu.cpp',
  'foobar2000/SDK/main_thread_callback.cpp',
  'foobar2000/SDK/mem_block_container.cpp',
  'foobar2000/SDK/menu_helpers.cpp',
  'foobar2000/SDK/menu_item.cpp',
  'foobar2000/SDK/menu_manager.cpp',
  'foobar2000/SDK/metadb.cpp',
  'foobar2000/SDK/metadb_handle.cpp',
  'foobar2000/SDK/metadb_handle_list.cpp',
  'foobar2000/SDK/output.cpp',
  'foobar2000/SDK/packet_decoder.cpp',
  'foobar2000/SDK/playable_location.cpp',
  'foobar2000/SDK/playback_control.cpp',
  'foobar2000/SDK/playlist.cpp',
  'foobar2000/SDK/playlist_loader.cpp',
  'foobar2000/SDK/popup_message.cpp',
  'foobar2000/SDK/preferences_page.cpp',
  'foobar2000/SDK/replaygain.cpp',
  'foobar2000/SDK/replaygain_info.cpp',
  'foobar2000/SDK/service.cpp',
  'foobar2000/SDK/stdafx.cpp',
  'foobar2000/SDK/tag_processor.cpp',
  'foobar2000/SDK/tag_processor_id3v2.cpp',
  'foobar2000/SDK/threaded_process.cpp',
  'foobar2000/SDK/titleformat.cpp',
  'foobar2000/SDK/track_property.cpp',
  'foobar2000/SDK/ui.cpp',
  'foobar2000/SDK/ui_element.cpp',
  'foobar2000/SDK/utility.cpp',
)

# Minimum official helper sources required by the external-CUE input route:
#  - cue_parser / cue_creator / cuesheet_index_list: the official CUE grammar parser.
#  - input_helpers + input_helper_cue: open the referenced audio file through normal
#    decoder services and decode a [start, length] track segment (recursion-safe redirect).
# Recorded in docs/architecture.md. Do not add helper sources that are not actually linked.
fb2k_helper_sources = files(
  'foobar2000/helpers/cue_parser.cpp',
  'foobar2000/helpers/cue_creator.cpp',
  'foobar2000/helpers/cuesheet_index_list.cpp',
  'foobar2000/helpers/input_helpers.cpp',
  'foobar2000/helpers/input_helper_cue.cpp',
  'foobar2000/helpers/file_list_helper.cpp',
  # readers.cpp provides fileCreateReadAhead / createFileMemMirrorAsync used by input_helpers.cpp.
  'foobar2000/helpers/readers.cpp',
)

pfc_lib = static_library(
  'pfc',
  pfc_sources,
  include_directories: sdk_inc,
  cpp_args: sdk_args + compile_defs + compat_args,
  cpp_pch: 'pfc/pfc-lite.h',
)

fb2k_sdk_lib = static_library(
  'foobar2000_SDK',
  fb2k_sdk_sources,
  include_directories: sdk_inc,
  dependencies: declare_dependency(link_with: pfc_lib),
  cpp_args: sdk_args + compile_defs + compat_args,
  cpp_pch: 'foobar2000/SDK/foobar2000-sdk-pch.h',
)

fb2k_helpers_lib = static_library(
  'foobar2000_helpers',
  fb2k_helper_sources,
  include_directories: sdk_inc,
  dependencies: declare_dependency(link_with: [pfc_lib, fb2k_sdk_lib]),
  cpp_args: sdk_args + compile_defs + compat_args,
)

component_client_lib = static_library(
  'foobar2000_component_client',
  files('foobar2000/foobar2000_component_client/component_client.cpp'),
  include_directories: sdk_inc,
  dependencies: declare_dependency(link_with: [pfc_lib, fb2k_sdk_lib]),
  cpp_args: sdk_args + compile_defs + compat_args,
)

shared_name = host_machine.cpu_family() == 'x86' ? 'shared-Win32' : 'shared-x64'
shared_dep = cpp.find_library(
  shared_name,
  dirs: meson.current_source_dir() / 'foobar2000/shared',
  required: true,
)

fb2k_sdk_dep = declare_dependency(
  include_directories: sdk_inc,
  link_with: [component_client_lib, fb2k_helpers_lib, fb2k_sdk_lib, pfc_lib],
  dependencies: [shared_dep],
)
