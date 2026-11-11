//! ICoreWebView2Settings w Ruście - pierwszy obiekt nowej architektury (docs/architektura-proxy.md).

use crate::api_gen::ICoreWebView2Settings9Impl;
use crate::protocol::*;
use crate::rpc::{Ctx, WvResult};
use std::cell::{Cell, RefCell};

/// Wartości domyślne jak w dokumentacji WebView2.
pub struct Settings {
    pub view: u32,
    flags: Cell<u32>,
    status_bar: Cell<bool>,
    host_objects: Cell<bool>,
    builtin_error_page: Cell<bool>,
    password_autosave: Cell<bool>,
    general_autofill: Cell<bool>,
    pinch_zoom: Cell<bool>,
    swipe_navigation: Cell<bool>,
    reputation_checking: Cell<bool>,
    non_client_region: Cell<bool>,
    hidden_pdf_items: Cell<i32>,
    user_agent: RefCell<Option<String>>,
}

impl Settings {
    pub fn new(view: u32) -> Self {
        Settings {
            view,
            flags: Cell::new(SETTINGS_DEFAULT),
            status_bar: Cell::new(true),
            host_objects: Cell::new(true),
            builtin_error_page: Cell::new(true),
            password_autosave: Cell::new(false),
            general_autofill: Cell::new(true),
            pinch_zoom: Cell::new(true),
            swipe_navigation: Cell::new(true),
            reputation_checking: Cell::new(true),
            non_client_region: Cell::new(false),
            hidden_pdf_items: Cell::new(0),
            user_agent: RefCell::new(None),
        }
    }

    fn flag(&self, f: u32) -> WvResult<bool> {
        Ok(self.flags.get() & f != 0)
    }

    fn set_flag(&self, f: u32, on: bool) -> WvResult<()> {
        let flags = if on { self.flags.get() | f } else { self.flags.get() & !f };
        if flags != self.flags.get() {
            self.flags.set(flags);
            crate::apply_settings(self.view, flags);
        }
        Ok(())
    }
}

macro_rules! flag_property {
    ($get:ident, $put:ident, $flag:expr) => {
        fn $get(&self, _ctx: &mut Ctx) -> WvResult<bool> {
            self.flag($flag)
        }
        fn $put(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
            self.set_flag($flag, value)
        }
    };
}

macro_rules! stored_property {
    ($get:ident, $put:ident, $field:ident) => {
        fn $get(&self, _ctx: &mut Ctx) -> WvResult<bool> {
            Ok(self.$field.get())
        }
        fn $put(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
            self.$field.set(value);
            Ok(())
        }
    };
}

impl ICoreWebView2Settings9Impl for Settings {
    flag_property!(get_IsScriptEnabled, put_IsScriptEnabled, SETTING_SCRIPT);
    flag_property!(get_IsWebMessageEnabled, put_IsWebMessageEnabled, SETTING_WEB_MESSAGE);
    flag_property!(get_AreDefaultScriptDialogsEnabled, put_AreDefaultScriptDialogsEnabled, SETTING_SCRIPT_DIALOGS);
    flag_property!(get_AreDefaultContextMenusEnabled, put_AreDefaultContextMenusEnabled, SETTING_CONTEXT_MENUS);
    flag_property!(get_AreDevToolsEnabled, put_AreDevToolsEnabled, SETTING_DEVTOOLS);
    flag_property!(get_IsZoomControlEnabled, put_IsZoomControlEnabled, SETTING_ZOOM);
    flag_property!(get_AreBrowserAcceleratorKeysEnabled, put_AreBrowserAcceleratorKeysEnabled, SETTING_ACCELERATORS);
    stored_property!(get_IsStatusBarEnabled, put_IsStatusBarEnabled, status_bar);
    stored_property!(get_AreHostObjectsAllowed, put_AreHostObjectsAllowed, host_objects);
    stored_property!(get_IsBuiltInErrorPageEnabled, put_IsBuiltInErrorPageEnabled, builtin_error_page);
    stored_property!(get_IsPasswordAutosaveEnabled, put_IsPasswordAutosaveEnabled, password_autosave);
    stored_property!(get_IsGeneralAutofillEnabled, put_IsGeneralAutofillEnabled, general_autofill);
    stored_property!(get_IsPinchZoomEnabled, put_IsPinchZoomEnabled, pinch_zoom);
    stored_property!(get_IsSwipeNavigationEnabled, put_IsSwipeNavigationEnabled, swipe_navigation);
    stored_property!(get_IsReputationCheckingRequired, put_IsReputationCheckingRequired, reputation_checking);
    stored_property!(get_IsNonClientRegionSupportEnabled, put_IsNonClientRegionSupportEnabled, non_client_region);

    fn get_UserAgent(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.user_agent.borrow().clone().unwrap_or_else(|| crate::DEFAULT_USER_AGENT.to_string()))
    }

    fn put_UserAgent(&self, _ctx: &mut Ctx, ua: Option<String>) -> WvResult<()> {
        let ua = ua.filter(|u| !u.is_empty());
        crate::apply_user_agent(self.view, ua.as_deref().unwrap_or(""));
        *self.user_agent.borrow_mut() = ua;
        Ok(())
    }

    fn get_HiddenPdfToolbarItems(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.hidden_pdf_items.get())
    }

    fn put_HiddenPdfToolbarItems(&self, _ctx: &mut Ctx, items: i32) -> WvResult<()> {
        self.hidden_pdf_items.set(items);
        Ok(())
    }
}
