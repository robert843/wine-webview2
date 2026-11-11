//! ContextMenuRequested: the menu CEF built, as WebView2 context menu items the application can
//! inspect, change (remove, insert its own items from CreateContextMenuItem) or handle itself.
//! What remains is shown by the DLL as a native popup menu (main.rs run_context_menu).

use crate::api_gen::*;
use crate::rpc::{Ctx, ObjRef, WvResult, E_INVALIDARG};
use crate::webview::{DeferralSlot, EventList};
use std::cell::{Cell, RefCell};
use std::rc::Rc;

// COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND
pub const KIND_COMMAND: i32 = 0;
pub const KIND_CHECK_BOX: i32 = 1;
pub const KIND_RADIO: i32 = 2;
pub const KIND_SEPARATOR: i32 = 3;
pub const KIND_SUBMENU: i32 = 4;

/// Command ids of custom items, above CEF's (MENU_ID_USER_LAST is 28500).
const CUSTOM_FIRST: i32 = 50000;

thread_local! {
    static NEXT_CUSTOM: Cell<i32> = const { Cell::new(CUSTOM_FIRST) };
}

pub struct MenuItem {
    name: String,
    label: String,
    command: i32,
    shortcut: String,
    icon: Option<Vec<u8>>,
    kind: i32,
    enabled: Cell<bool>,
    checked: Cell<bool>,
    children: Option<ObjRef>,
    custom_selected: EventList,
}

pub type ItemObj = ICoreWebView2ContextMenuItemObj<MenuItem>;
type CollectionObj = ICoreWebView2ContextMenuItemCollectionObj<ItemCollection>;

impl MenuItem {
    pub fn is_custom(&self) -> bool {
        self.command >= CUSTOM_FIRST
    }
}

impl ICoreWebView2ContextMenuItemImpl for MenuItem {
    fn get_Name(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.name.clone())
    }
    fn get_Label(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.label.clone())
    }
    fn get_CommandId(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.command)
    }
    fn get_ShortcutKeyDescription(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.shortcut.clone())
    }
    fn get_Icon(&self, _ctx: &mut Ctx) -> WvResult<Option<Vec<u8>>> {
        Ok(self.icon.clone())
    }
    fn get_Kind(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.kind)
    }
    fn put_IsEnabled(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.enabled.set(value);
        Ok(())
    }
    fn get_IsEnabled(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.enabled.get())
    }
    fn put_IsChecked(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.checked.set(value);
        Ok(())
    }
    fn get_IsChecked(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.checked.get())
    }
    fn get_Children(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.children.clone().unwrap_or_default())
    }
    fn add_CustomItemSelected(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        self.custom_selected.add(handler)
    }
    fn remove_CustomItemSelected(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
        self.custom_selected.remove(token)
    }
}

#[derive(Default)]
pub struct ItemCollection {
    items: RefCell<Vec<ObjRef>>,
}

impl ICoreWebView2ContextMenuItemCollectionImpl for ItemCollection {
    fn get_Count(&self, _ctx: &mut Ctx) -> WvResult<u32> {
        Ok(self.items.borrow().len() as u32)
    }
    fn GetValueAtIndex(&self, _ctx: &mut Ctx, index: u32) -> WvResult<ObjRef> {
        self.items.borrow().get(index as usize).cloned().ok_or(E_INVALIDARG)
    }
    fn RemoveValueAtIndex(&self, _ctx: &mut Ctx, index: u32) -> WvResult<()> {
        let mut items = self.items.borrow_mut();
        if index as usize >= items.len() {
            return Err(E_INVALIDARG);
        }
        items.remove(index as usize);
        Ok(())
    }
    fn InsertValueAtIndex(&self, _ctx: &mut Ctx, index: u32, value: ObjRef) -> WvResult<()> {
        // Only items of this runtime (from the menu or CreateContextMenuItem).
        if value.downcast::<ItemObj>().is_none() {
            return Err(E_INVALIDARG);
        }
        let mut items = self.items.borrow_mut();
        if index as usize > items.len() {
            return Err(E_INVALIDARG);
        }
        items.insert(index as usize, value);
        Ok(())
    }
}

fn collection(items: Vec<ObjRef>) -> ObjRef {
    ObjRef::Runtime(Rc::new(ICoreWebView2ContextMenuItemCollectionObj(ItemCollection { items: RefCell::new(items) })))
}

fn item(name: &str, label: &str, command: i32, kind: i32, enabled: bool, checked: bool, children: Option<ObjRef>,
        icon: Option<Vec<u8>>) -> ObjRef {
    ObjRef::Runtime(Rc::new(ICoreWebView2ContextMenuItemObj(MenuItem {
        name: name.to_string(),
        label: label.to_string(),
        command,
        shortcut: String::new(),
        icon,
        kind,
        enabled: Cell::new(enabled),
        checked: Cell::new(checked),
        children,
        custom_selected: EventList::default(),
    })))
}

/// ICoreWebView2Environment9::CreateContextMenuItem.
pub fn create_custom_item(label: &str, icon: Option<Vec<u8>>, kind: i32) -> WvResult<ObjRef> {
    if !(KIND_COMMAND..=KIND_SUBMENU).contains(&kind) {
        return Err(E_INVALIDARG);
    }
    let command = NEXT_CUSTOM.with(|n| {
        let id = n.get();
        n.set(id + 1);
        id
    });
    let children = (kind == KIND_SUBMENU).then(|| collection(Vec::new()));
    Ok(item("custom", label, command, kind, true, false, children, icon))
}

/// WebView2's item name for a CEF command id.
fn item_name(command: i32) -> &'static str {
    match command {
        100 => "back",
        101 => "forward",
        102 | 103 => "reload",
        110 => "undo",
        111 => "redo",
        112 => "cut",
        113 => "copy",
        114 => "paste",
        115 => "pasteAsPlainText",
        117 => "selectAll",
        131 => "print",
        200..=204 => "spellcheck",
        26501 => "inspectElement",
        _ => "other",
    }
}

/// One entry of CEF's model for the item list (None: not representable).
pub struct ModelEntry {
    pub command: i32,
    pub label: String,
    pub kind: i32,
    pub enabled: bool,
    pub checked: bool,
}

pub fn items_from_model(entries: Vec<ModelEntry>) -> Vec<ObjRef> {
    entries
        .into_iter()
        .map(|e| {
            let name = if e.kind == KIND_SEPARATOR { "separator" } else { item_name(e.command) };
            item(name, &e.label, e.command, e.kind, e.enabled, e.checked, None, None)
        })
        .collect()
}

pub struct Target {
    pub kind: i32,
    pub editable: bool,
    pub main_frame: bool,
    pub page_uri: String,
    pub frame_uri: String,
    pub link_uri: String,
    pub link_text: String,
    pub source_uri: String,
    pub selection: String,
}

impl ICoreWebView2ContextMenuTargetImpl for Target {
    fn get_Kind(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.kind)
    }
    fn get_IsEditable(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.editable)
    }
    fn get_IsRequestedForMainFrame(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.main_frame)
    }
    fn get_PageUri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.page_uri.clone())
    }
    fn get_FrameUri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.frame_uri.clone())
    }
    fn get_HasLinkUri(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(!self.link_uri.is_empty())
    }
    fn get_LinkUri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.link_uri.clone())
    }
    fn get_HasLinkText(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(!self.link_text.is_empty())
    }
    fn get_LinkText(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.link_text.clone())
    }
    fn get_HasSourceUri(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(!self.source_uri.is_empty())
    }
    fn get_SourceUri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.source_uri.clone())
    }
    fn get_HasSelection(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(!self.selection.is_empty())
    }
    fn get_SelectionText(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.selection.clone())
    }
}

pub struct RequestedArgs {
    items: ObjRef,
    target: ObjRef,
    location: POINT,
    selected: Cell<i32>,
    handled: Cell<bool>,
    deferral: Rc<DeferralSlot>,
}

impl ICoreWebView2ContextMenuRequestedEventArgsImpl for RequestedArgs {
    fn get_MenuItems(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.items.clone())
    }
    fn get_ContextMenuTarget(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.target.clone())
    }
    fn get_Location(&self, _ctx: &mut Ctx) -> WvResult<POINT> {
        Ok(self.location)
    }
    fn put_SelectedCommandId(&self, _ctx: &mut Ctx, value: i32) -> WvResult<()> {
        self.selected.set(value);
        Ok(())
    }
    fn get_SelectedCommandId(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.selected.get())
    }
    fn put_Handled(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.handled.set(value);
        Ok(())
    }
    fn get_Handled(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.handled.get())
    }
    fn GetDeferral(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.deferral.get())
    }
}

/// What to do with the menu once the application's handlers (and deferral) are done.
pub enum Decision {
    /// Show these items (the possibly changed MenuItems).
    Show(Vec<ObjRef>),
    /// Handled with SelectedCommandId: run it without a menu.
    Run(ObjRef),
    Cancel,
}

pub fn find_command(items: &[ObjRef], command: i32) -> Option<ObjRef> {
    for item in items {
        let Some(i) = item.downcast::<ItemObj>() else { continue };
        if i.0.command == command && i.0.kind != KIND_SEPARATOR && i.0.kind != KIND_SUBMENU {
            return Some(item.clone());
        }
        if let Some(children) = i.0.children.as_ref().and_then(|c| c.downcast::<CollectionObj>()) {
            if let Some(found) = find_command(&children.0.items.borrow(), command) {
                return Some(found);
            }
        }
    }
    None
}

/// Raises ContextMenuRequested (sender, handlers from webview.rs); `decide` gets the outcome,
/// later if the application took a deferral (without handlers: show the menu as it is).
pub fn raise(view: u32, items: Vec<ObjRef>, target: Target, location: POINT, decide: Box<dyn FnOnce(Decision)>) {
    let (handlers, sender) = crate::webview::context_menu_handlers(view).unwrap_or_default();
    if handlers.is_empty() {
        return decide(Decision::Show(items));
    }
    let args = Rc::new(ICoreWebView2ContextMenuRequestedEventArgsObj(RequestedArgs {
        items: collection(items),
        target: ObjRef::Runtime(Rc::new(ICoreWebView2ContextMenuTargetObj(target))),
        location,
        selected: Cell::new(-1),
        handled: Cell::new(false),
        deferral: DeferralSlot::new(),
    }));
    let args_ref = ObjRef::Runtime(args.clone());
    for h in handlers {
        if let Err(e) = ICoreWebView2ContextMenuRequestedEventHandlerApp(h).Invoke(&mut Ctx, &sender, &args_ref) {
            eprintln!("helper: ContextMenuRequested handler failed: {e:#x}");
        }
    }
    let finish = {
        let args = args.clone();
        move || {
            let a = &args.0;
            let items = a.items.downcast::<CollectionObj>().map(|c| c.0.items.borrow().clone()).unwrap_or_default();
            let decision = if a.handled.get() {
                match find_command(&items, a.selected.get()) {
                    Some(item) => Decision::Run(item),
                    None => Decision::Cancel,
                }
            } else {
                Decision::Show(items)
            };
            decide(decision);
        }
    };
    let pending: Rc<RefCell<Option<Box<dyn FnOnce()>>>> = Rc::new(RefCell::new(Some(Box::new(finish))));
    let later = pending.clone();
    if args.0.deferral.settle(Box::new(move || {
        if let Some(f) = later.borrow_mut().take() {
            f()
        }
    })) {
        if let Some(f) = pending.borrow_mut().take() {
            f()
        }
    }
}

/// A custom item was chosen: CustomItemSelected on it (sender = the item, args = null).
pub fn custom_item_selected(item: &ObjRef) {
    let Some(i) = item.downcast::<ItemObj>() else { return };
    for h in i.0.custom_selected.snapshot() {
        ICoreWebView2CustomItemSelectedEventHandlerApp(h).Invoke_async(&mut Ctx, item, &ObjRef::Null);
    }
}

/// Item fields for building the native menu: (kind, command, label, enabled, checked, children).
pub fn describe(item: &ObjRef) -> Option<(i32, i32, String, bool, bool, Vec<ObjRef>)> {
    let i = item.downcast::<ItemObj>()?;
    let children = i.0.children.as_ref().and_then(|c| c.downcast::<CollectionObj>()).map(|c| c.0.items.borrow().clone());
    Some((i.0.kind, i.0.command, i.0.label.clone(), i.0.enabled.get(), i.0.checked.get(), children.unwrap_or_default()))
}
