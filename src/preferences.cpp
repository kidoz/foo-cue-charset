#include "preferences.hpp"

#include "config.hpp"
#include "resource.h"

#include <SDK/preferences_page.h>

#include <windows.h>

namespace foo_cue_charset::preferences {

// {1177CA81-E67A-41A6-8EAB-7A232C034111}
const GUID guid_preferences_page = {0x1177ca81, 0xe67a, 0x41a6, {0x8e, 0xab, 0x7a, 0x23, 0x2c, 0x03, 0x41, 0x11}};

namespace {

const wchar_t* const kLegacyNames[] = {L"Windows-1251", L"KOI8-R", L"CP866", L"ISO-8859-5"};

// A preferences page instance backed by a plain Win32 child dialog (no WTL dependency).
class prefs_instance : public preferences_page_instance {
 public:
  void create(fb2k::hwnd_t parent, preferences_page_callback::ptr callback) {
    m_callback = callback;
    m_wnd = CreateDialogParamW(core_api::get_my_instance(), MAKEINTRESOURCEW(IDD_PREFERENCES), parent, dialog_proc,
                               reinterpret_cast<LPARAM>(this));
  }

  t_uint32 get_state() override {
    t_uint32 state = static_cast<t_uint32>(preferences_state::resettable) |
                     static_cast<t_uint32>(preferences_state::dark_mode_supported);
    if (has_changes()) {
      state |= static_cast<t_uint32>(preferences_state::changed);
    }
    return state;
  }

  fb2k::hwnd_t get_wnd() override { return m_wnd; }

  void apply() override {
    config::set_mode(ui_mode());
    config::set_legacy_index(ui_legacy_index());
    config::set_logging(ui_logging());
    notify_changed();
  }

  void reset() override {
    set_controls(config::default_mode, config::default_legacy_index, config::default_logging);
    notify_changed();
  }

 private:
  bool has_changes() const {
    if (m_wnd == nullptr) {
      return false;
    }
    return ui_mode() != config::get_mode() || ui_legacy_index() != config::get_legacy_index() ||
           ui_logging() != config::get_logging();
  }

  int ui_mode() const {
    return (IsDlgButtonChecked(m_wnd, IDC_MODE_FORCE) == BST_CHECKED) ? config::mode_force_selected
                                                                      : config::mode_automatic;
  }

  int ui_legacy_index() const {
    const LRESULT sel = SendDlgItemMessageW(m_wnd, IDC_LEGACY_ENCODING, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR || sel < 0 || sel >= config::legacy_choice_count) {
      return config::default_legacy_index;
    }
    return static_cast<int>(sel);
  }

  bool ui_logging() const { return IsDlgButtonChecked(m_wnd, IDC_LOG) == BST_CHECKED; }

  void set_controls(int mode, int legacy_index, bool logging) {
    CheckRadioButton(m_wnd, IDC_MODE_AUTOMATIC, IDC_MODE_FORCE,
                     mode == config::mode_force_selected ? IDC_MODE_FORCE : IDC_MODE_AUTOMATIC);
    SendDlgItemMessageW(m_wnd, IDC_LEGACY_ENCODING, CB_SETCURSEL, static_cast<WPARAM>(legacy_index), 0);
    CheckDlgButton(m_wnd, IDC_LOG, logging ? BST_CHECKED : BST_UNCHECKED);
  }

  void notify_changed() {
    if (m_callback.is_valid()) {
      m_callback->on_state_changed();
    }
  }

  void on_init() {
    for (const wchar_t* name : kLegacyNames) {
      SendDlgItemMessageW(m_wnd, IDC_LEGACY_ENCODING, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
    }
    set_controls(config::get_mode(), config::get_legacy_index(), config::get_logging());
  }

  static INT_PTR CALLBACK dialog_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_INITDIALOG) {
      auto* self = reinterpret_cast<prefs_instance*>(lparam);
      SetWindowLongPtrW(wnd, DWLP_USER, static_cast<LONG_PTR>(lparam));
      self->m_wnd = wnd;
      self->on_init();
      return TRUE;
    }

    auto* self = reinterpret_cast<prefs_instance*>(GetWindowLongPtrW(wnd, DWLP_USER));
    if (self == nullptr) {
      return FALSE;
    }

    if (msg == WM_COMMAND) {
      const int id = LOWORD(wparam);
      const int code = HIWORD(wparam);
      const bool changed =
          ((id == IDC_MODE_AUTOMATIC || id == IDC_MODE_FORCE || id == IDC_LOG) && code == BN_CLICKED) ||
          (id == IDC_LEGACY_ENCODING && code == CBN_SELCHANGE);
      if (changed) {
        self->notify_changed();
      }
      return FALSE;
    }
    return FALSE;
  }

  fb2k::hwnd_t m_wnd = nullptr;
  preferences_page_callback::ptr m_callback;
};

class prefs_page : public preferences_page_v3 {
 public:
  const char* get_name() override { return "CUE Charset"; }
  GUID get_guid() override { return guid_preferences_page; }
  GUID get_parent_guid() override { return preferences_page::guid_tools; }

  preferences_page_instance::ptr instantiate(fb2k::hwnd_t parent, preferences_page_callback::ptr callback) override {
    auto instance = fb2k::service_new<prefs_instance>();
    instance->create(parent, callback);
    return instance;
  }
};

static preferences_page_factory_t<prefs_page> g_prefs_page_factory;

} // namespace

} // namespace foo_cue_charset::preferences
